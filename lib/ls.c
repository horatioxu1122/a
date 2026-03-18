/* tmux session list helper */
static int tm_list(char out[B], char *lines[], int max) {
    pcmd("tmux list-sessions -F '#{session_name}' 2>/dev/null", out, B);
    int n=0; char *p=out;
    while (*p && n < max) { lines[n++]=p; char *e=strchr(p,'\n'); if(e){*e=0;p=e+1;}else break; }
    return n;
}
/* ── ls ── */
static int cmd_ls(int argc, char **argv) {
    char out[B]; char *lines[64]; int n=tm_list(out,lines,64);
    if (argc > 2 && argv[2][0] >= '0' && argv[2][0] <= '9') {
        int idx = atoi(argv[2]);
        if (idx >= 0 && idx < n) tm_go(lines[idx]);
        return 0;
    }
    if (!n) { puts("No sessions"); return 0; }
    for (int i = 0; i < n; i++) {
        char c[B], path[512] = "";
        snprintf(c, B, "tmux display-message -p -t '%s' '#{pane_current_path}' 2>/dev/null", lines[i]);
        pcmd(c, path, 512); path[strcspn(path,"\n")] = 0;
        printf("  %d  %s: %s\n", i, lines[i], path);
    }
    puts("\nSelect:\n  a ls 0"); return 0;
}

/* ── kill ── */
static int cmd_kill(int argc, char **argv) {
    const char *sel = argc > 2 ? argv[2] : NULL;
    if ((sel && !strcmp(sel, "all")) || (argc > 1 && !strcmp(argv[1], "killall"))) {
        (void)!system("pkill -9 -f tmux 2>/dev/null"); (void)!system("clear");
        puts("\xe2\x9c\x93"); return 0;
    }
    char out[B]; char *lines[64]; int n=tm_list(out,lines,64);
    if (!n) { puts("No sessions"); return 0; }
    if (sel && sel[0] >= '0' && sel[0] <= '9') {
        int idx = atoi(sel);
        if (idx >= 0 && idx < n) {
            char c[B]; snprintf(c, B, "tmux kill-session -t '%s'", lines[idx]); (void)!system(c);
            printf("\xe2\x9c\x93 %s\n", lines[idx]); return 0;
        }
    }
    for (int i = 0; i < n; i++) printf("  %d  %s\n", i, lines[i]);
    puts("\nSelect:\n  a kill 0\n  a kill all"); return 0;
}

static int cmd_copy(int c,char**v){(void)c;(void)v;char o[B];int ol=0;
    if(!isatty(0)){ssize_t n;while((n=read(0,o+ol,(size_t)(B-ol-1)))>0)ol+=(int)n;}
    else if(getenv("TMUX")){pcmd("tmux capture-pane -pJ -S-99|awk '/[$@].*[$@]|❯/{b=s;s=\"\";next}{s=s?s\"\\n\"$0:$0}END{printf\"%s\",b}'",o,B);ol=(int)strlen(o);}
    else{puts("x Pipe or tmux");return 1;}
    if(ol<1){puts("x No output");return 0;}o[ol]=0;if(to_clip(o)){puts("x Needs tmux");return 1;}printf("\xe2\x9c\x93 %.50s\n",o);return 0;}

/* ── dash ── */
static int cmd_dash_tui(void);
static int cmd_dash_input(void);
static int cmd_dash(int c, char **v) { (void)c;(void)v;
    perf_disarm();
    if(c>2&&!strcmp(v[2],"--tui"))return cmd_dash_tui();
    if(c>2&&!strcmp(v[2],"--input"))return cmd_dash_input();
    char cm[B];
    if(!tm_has("dash")){CWD(wd);
        snprintf(cm,B,"tmux new-session -d -s dash -c '%s' '%s/a dash --tui'",wd,DDIR);
        (void)!system(cm);}
    tm_go("dash");return 0;
}
static void dash_refresh(char out[B],char**lines,int*n){
    *n=tm_list(out,lines,64);
    for(int i=0;i<*n;i++)if(!strcmp(lines[i],"dash")){for(int j=i;j<*n-1;j++)lines[j]=lines[j+1];(*n)--;i--;}
}
static int dash_panes(const char *sess, char ids[][16], int max) {
    char cm[B],buf[256];
    snprintf(cm,B,"tmux list-panes -t '%s':0 -F '#{pane_id}' 2>/dev/null",sess);
    pcmd(cm,buf,256);int n=0;
    for(char*p=buf;*p&&n<max;){char*e=strchr(p,'\n');if(e)*e=0;
        if(*p)snprintf(ids[n++],16,"%s",p);if(e)p=e+1;else break;}
    return n;
}
static void dash_restore(char cm[B],char tp[][16],char rp[][16],int ntp){
    for(int i=0;i<ntp;i++){snprintf(cm,B,"tmux swap-pane -s %s -t %s 2>/dev/null",tp[i],rp[i]);(void)!system(cm);}
}
/* bottom-left: rapid job launcher, creates sessions */
static int cmd_dash_input(void) {
    init_db();load_cfg();load_proj();
    char ln[B];CWD(wd);
    for(fputs("j> ",stdout),fflush(stdout);fgets(ln,B,stdin);fputs("j> ",stdout),fflush(stdout)){
        ln[strcspn(ln,"\n")]=0;if(!ln[0])continue;
        char sn[64];time_t t=time(NULL);struct tm*tm=localtime(&t);
        snprintf(sn,64,"j-%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
        char jcmd[B];jcmd_fill(jcmd,0);
        tm_new(sn,wd,jcmd);
        sleep(1);/* let session + split settle */
        send_prefix_bg(sn,"claude",wd,ln);
        printf("+ %s\n",sn);
    }
    return 0;
}
/* top-left: session list + swap controller */
static int cmd_dash_tui(void) {
    char out[B],*lines[64],cm[B];int n,sel=0;
    char me[16]="",rp[2][16]={"",""},tp[2][16]={"",""};int ntp=0;
    pcmd("tmux display -p '#{pane_id}'",me,16);me[strcspn(me,"\n")]=0;
    /* create right panes from full-height left, then split left for input */
    snprintf(cm,B,"tmux split-window -hd -t %s -p 80 -PF '#{pane_id}'",me);
    pcmd(cm,rp[0],16);rp[0][strcspn(rp[0],"\n")]=0;
    snprintf(cm,B,"tmux split-window -vd -t %s -p 40 -PF '#{pane_id}'",rp[0]);
    pcmd(cm,rp[1],16);rp[1][strcspn(rp[1],"\n")]=0;
    snprintf(cm,B,"tmux split-window -vd -t %s -p 30 '%s/a dash --input'",me,DDIR);
    (void)!system(cm);
    snprintf(cm,B,"tmux select-pane -t %s",me);(void)!system(cm);
    /* event-driven: tmux hooks signal us on session create/close */
    signal(SIGUSR1,SIG_IGN);/* just interrupt select */
    snprintf(cm,B,"tmux set-hook -g session-created 'run-shell -b \"kill -USR1 %d 2>/dev/null\"'",(int)getpid());(void)!system(cm);
    snprintf(cm,B,"tmux set-hook -g session-closed 'run-shell -b \"kill -USR1 %d 2>/dev/null\"'",(int)getpid());(void)!system(cm);
    struct termios old,raw_t;tcgetattr(0,&old);raw_t=old;
    raw_t.c_lflag&=~(tcflag_t)(ICANON|ECHO|ISIG);raw_t.c_cc[VMIN]=1;raw_t.c_cc[VTIME]=0;
    tcsetattr(0,TCSANOW,&raw_t);
    write(1,"\033[?1000h\033[?1006h",16);
    dash_refresh(out,lines,&n);
    #define DASH_SWAP() do { \
        dash_restore(cm,tp,rp,ntp); \
        char ids[8][16];int np=dash_panes(lines[sel],ids,8);if(np>2)np=2; \
        for(int _i=0;_i<np;_i++){snprintf(cm,B,"tmux swap-pane -s %s -t %s",rp[_i],ids[_i]);(void)!system(cm);snprintf(tp[_i],16,"%s",ids[_i]);} \
        ntp=np; \
        snprintf(cm,B,"tmux select-pane -t %s",me);(void)!system(cm); \
    } while(0)
    if(n>0)DASH_SWAP();
    for(;;){
        struct winsize ws;ioctl(0,TIOCGWINSZ,&ws);int rows=ws.ws_row;
        {char fb[B*2];int fl=0;
        fl+=snprintf(fb+fl,(size_t)(B*2-fl),"\033[2J\033[H\033[?25l");
        for(int i=0;i<rows-1&&i<n;i++)
            fl+=snprintf(fb+fl,(size_t)(B*2-fl),"%s  %s\033[0m\033[K\n",i==sel?"\033[7m":"",lines[i]);
        fl+=snprintf(fb+fl,(size_t)(B*2-fl),"\033[%d;1H\033[7m j/k:nav x:kill q:quit\033[0m\033[K",rows);
        (void)!write(1,fb,(size_t)fl);}
        char ch;
        fd_set fds;struct timeval tv={30,0};FD_ZERO(&fds);FD_SET(0,&fds);
        {int sr=select(1,&fds,0,0,&tv);
        if(sr<=0){dash_refresh(out,lines,&n);sel=sel<n?sel:n-1;if(sel<0)sel=0;continue;}}
        if(read(0,&ch,1)!=1)break;
        int do_pick=0;
        if(ch=='\x1b'){int av;usleep(50000);ioctl(0,FIONREAD,&av);if(!av)break;
            char seq[2];if(read(0,seq,1)!=1)break;
            if(seq[0]=='['){if(read(0,seq+1,1)!=1)break;
                if(seq[1]=='A'){sel=sel>0?sel-1:n-1;do_pick=1;}
                else if(seq[1]=='B'){sel=sel<n-1?sel+1:0;do_pick=1;}
                else if(seq[1]=='<'){int mb=0,mx=0,my=0;char mc;
                    while(read(0,&mc,1)==1&&mc!=';')mb=mb*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!=';')mx=mx*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!='M'&&mc!='m')my=my*10+mc-'0';
                    (void)mx;
                    if(mc=='M'&&mb==0&&my-1>=0&&my-1<n){sel=my-1;do_pick=1;}
                    if(mc=='M'&&mb==64&&sel>0){sel--;do_pick=1;}
                    if(mc=='M'&&mb==65&&sel<n-1){sel++;do_pick=1;}
                }}}
        if(ch=='q'||ch==3)break;
        if(ch=='k'){sel=sel>0?sel-1:n-1;do_pick=1;}
        else if(ch=='j'){sel=sel<n-1?sel+1:0;do_pick=1;}
        else if(ch=='\r'||ch=='\n')do_pick=1;
        else if(ch=='x'&&n>0){
            dash_restore(cm,tp,rp,ntp);ntp=0;tp[0][0]=tp[1][0]=0;
            char cm2[B];snprintf(cm2,B,"tmux kill-session -t '=%s' 2>/dev/null",lines[sel]);(void)!system(cm2);
            dash_refresh(out,lines,&n);sel=sel<n?sel:n-1;if(sel<0)sel=0;
            if(n>0)DASH_SWAP();
        }
        if(do_pick&&n>0)DASH_SWAP();
        dash_refresh(out,lines,&n);sel=sel<n?sel:n-1;if(sel<0)sel=0;
    }
    #undef DASH_SWAP
    dash_restore(cm,tp,rp,ntp);
    for(int i=0;i<2;i++)if(rp[i][0]){snprintf(cm,B,"tmux kill-pane -t %s 2>/dev/null",rp[i]);(void)!system(cm);}
    (void)!system("tmux set-hook -gu session-created 2>/dev/null");
    (void)!system("tmux set-hook -gu session-closed 2>/dev/null");
    write(1,"\033[?1000l\033[?1006l",16);
    tcsetattr(0,TCSANOW,&old);printf("\033[2J\033[H\033[?25h");return 0;
}

/* ── watch ── */
static int cmd_watch(int argc, char **argv) {
    if (argc < 3) { puts("Usage: a watch <session> [duration]"); return 1; }
    const char *sn = argv[2]; int dur = argc > 3 ? atoi(argv[3]) : 0;
    printf("Watching '%s'%s\n", sn, dur ? "" : " (once)");
    time_t start = time(NULL);
    char last[B] = "";
    while (1) {
        if (dur && time(NULL) - start > dur) break;
        char out[B];
        if (tm_read(sn, out, B) != 0) { printf("x Session %s not found\n", sn); return 1; }
        if (strcmp(out, last)) {
            if (strstr(out, "Are you sure?") || strstr(out, "Continue?") || strstr(out, "[y/N]") || strstr(out, "[Y/n]")) {
                tm_key(sn, "y"); tm_key(sn, "Enter");
                puts("\xe2\x9c\x93 Auto-responded");
            }
            snprintf(last, B, "%s", out);
        }
        usleep(100000);
        if (!dur) break;
    }
    return 0;
}

/* ── send ── */
static int cmd_send(int argc, char **argv) {
    if (argc < 4) { puts("Usage: a send <session> <prompt> [--wait] [--no-enter]"); return 1; }
    const char *sn = argv[2];
    if (!tm_has(sn)) { printf("x Session %s not found\n", sn); return 1; }
    char prompt[B]=""; int wait=0,enter=1;
    {int pl=0;for(int i=3;i<argc;i++){
        if(!strcmp(argv[i],"--wait"))wait=1;
        else if(!strcmp(argv[i],"--no-enter"))enter=0;
        else pl+=snprintf(prompt+pl,(size_t)(B-pl),"%s%s",pl?" ":"",argv[i]);}}
    tm_send(sn, prompt);
    if (enter) { usleep(100000); tm_key(sn, "Enter"); }
    printf("\xe2\x9c\x93 %s '%s'\n", enter?"Sent to":"Inserted into", sn);
    if (wait) {
        printf("Waiting..."); fflush(stdout);
        time_t last_active = time(NULL);
        while (1) {
            char c[B]; snprintf(c, B, "tmux display-message -p -t '%s' '#{window_activity}' 2>/dev/null", sn);
            char out[64]; pcmd(c, out, 64);
            int act = atoi(out);
            if (time(NULL) - act < 2) { last_active = time(NULL); printf("."); fflush(stdout); }
            else if (time(NULL) - last_active > 3) { puts("\n+ Done"); break; }
            usleep(500000);
        }
    }
    return 0;
}

/* ── jobs ── active panes (local+remote) + review worktrees */
typedef struct{char n[64],p[256];time_t mt;}wtr_t;
static int wtr_cmp(const void*a,const void*b){time_t d=((const wtr_t*)b)->mt-((const wtr_t*)a)->mt;return d>0?1:d<0?-1:0;}
typedef struct{char sn[64],pid[32],cmd[32],p[128],dev[32];}jpane_t;
static int cmd_jobs(int argc, char **argv) {
    const char *sel=NULL,*rm=NULL;
    for(int i=2;i<argc;i++){if(!strcmp(argv[i],"rm")&&i+1<argc)rm=argv[++i];
        else if(!strcmp(argv[i],"watch")){perf_disarm();execlp("watch","watch","-n2","-c","a","job",(char*)0);return 0;}
        else if(strcmp(argv[i],"-r")&&strcmp(argv[i],"--running"))sel=argv[i];}
    init_db();load_cfg();
    jpane_t A[64];int na=0;
    /* Local windows */
    char out[B*2];pcmd("tmux list-windows -a -F '#{session_name}\t#{window_id}\t#{pane_current_command}\t#{pane_current_path}' 2>/dev/null",out,B*2);
    for(char*p=out;*p&&na<64;){char*e=strchr(p,'\n');if(e)*e=0;
        char*t1=strchr(p,'\t'),*t2=t1?strchr(t1+1,'\t'):0,*t3=t2?strchr(t2+1,'\t'):0;
        if(t1&&t2&&t3){*t1=*t2=*t3=0;
            snprintf(A[na].sn,64,"%s",p);snprintf(A[na].pid,32,"%s",t1+1);
            snprintf(A[na].cmd,32,"%s",t2+1);snprintf(A[na].p,128,"%s",bname(t3+1));snprintf(A[na].dev,32,"%s",DEV);na++;}
        if(e)p=e+1;else break;}
    /* Remote panes: read cache, bg refresh */
    {char cf[P];snprintf(cf,P,"%s/job_remote.cache",DDIR);
    char*dat=readf(cf,NULL);if(dat){
        for(char*rp=dat;*rp&&na<64;){char*re=strchr(rp,'\n');if(re)*re=0;
            char*d1=strchr(rp,'|'),*r1=d1?strchr(d1+1,'|'):0,*r2=r1?strchr(r1+1,'|'):0;
            if(d1&&r1&&r2){*d1=*r1=*r2=0;
                int dup=0;for(int j=0;j<na;j++)if(!strcmp(A[j].sn,d1+1)&&!strcmp(A[j].dev,rp)){dup=1;break;}
                if(!dup){snprintf(A[na].sn,64,"%s",d1+1);A[na].pid[0]=0;
                snprintf(A[na].cmd,32,"%s",r1+1);snprintf(A[na].p,128,"%s",bname(r2+1));
                snprintf(A[na].dev,32,"%s",rp);na++;}}
            if(re)rp=re+1;else break;}free(dat);}}
    {pid_t bg=fork();if(bg==0){close(0);close(1);close(2);
        char sdir[P];snprintf(sdir,P,"%s/ssh",SROOT);
        char hp[32][P];int nh=listdir(sdir,hp,32);
        struct{char hn[64];int fd;pid_t pid;}SP[16];int nsp=0;
        for(int h=0;h<nh&&nsp<16;h++){
            kvs_t kv=kvfile(hp[h]);const char*hn=kvget(&kv,"Name");
            if(!hn||!strcmp(hn,DEV))continue;
            int pfd[2];if(pipe(pfd))continue;
            pid_t p=fork();if(p==0){close(pfd[0]);
                char sc[B];snprintf(sc,B,"a ssh %s 'tmux list-windows -a -F \"#{session_name}|#{pane_current_command}|#{pane_current_path}\"' 2>/dev/null",hn);
                FILE*f=popen(sc,"r");if(f){char buf[B];size_t r=fread(buf,1,B-1,f);buf[r]=0;(void)!write(pfd[1],buf,r);pclose(f);}
                close(pfd[1]);_exit(0);}
            close(pfd[1]);snprintf(SP[nsp].hn,64,"%s",hn);SP[nsp].fd=pfd[0];SP[nsp].pid=p;nsp++;}
        char cf[P],tmp[P];snprintf(cf,P,"%s/job_remote.cache",DDIR);snprintf(tmp,P,"%s.%d",cf,getpid());
        FILE*fo=fopen(tmp,"w");
        for(int s=0;s<nsp;s++){
            char ro[B];int len=(int)read(SP[s].fd,ro,B-1);ro[len>0?len:0]=0;close(SP[s].fd);waitpid(SP[s].pid,NULL,0);
            for(char*rp=ro;*rp;){char*re=strchr(rp,'\n');if(re)*re=0;
                if(strchr(rp,'|')&&fo)fprintf(fo,"%s|%s\n",SP[s].hn,rp);
                if(re){rp=re+1;}else break;}}
        if(fo){fclose(fo);rename(tmp,cf);}
        _exit(0);}}
    /* Review worktrees */
    char wd[P];{const char*w=cfget("worktrees_dir");if(w[0])snprintf(wd,P,"%s",w);else snprintf(wd,P,"%s/worktrees",AROOT);}
    wtr_t R[32];int nr=0;
    if(dexists(wd)){DIR*d=opendir(wd);struct dirent*de;if(d){while((de=readdir(d))&&nr<32){
        if(de->d_name[0]=='.')continue;char fp[P];snprintf(fp,P,"%s/%s",wd,de->d_name);
        if(!dexists(fp))continue;
        char df[P];snprintf(df,P,"%s/.a_done",fp);int done=fexists(df);
        int act=0;if(!done)for(int i=0;i<na;i++)if(A[i].pid[0]&&!strcmp(bname(fp),A[i].p)){act=1;break;}if(act)continue;
        snprintf(R[nr].n,64,"%s",de->d_name);snprintf(R[nr].p,256,"%s",fp);
        struct stat st;R[nr].mt=(!stat(fp,&st))?st.st_mtime:0;
        nr++;}closedir(d);}
    qsort(R,(size_t)nr,sizeof(wtr_t),wtr_cmp);}
    if(rm&&!strcmp(rm,"all")){for(int i=0;i<nr;i++){char c[B];snprintf(c,B,"rm -rf '%s'",R[i].p);(void)!system(c);}
        printf("\xe2\x9c\x93 %d worktrees\n",nr);return 0;}
    if(rm&&*rm>='0'&&*rm<='9'){int x=atoi(rm);
        if(x<na&&A[x].pid[0]){char c[B];snprintf(c,B,"tmux kill-window -t '%s'",A[x].pid);(void)!system(c);printf("\xe2\x9c\x93 %s\n",A[x].sn);}
        else if(x-na>=0&&x-na<nr){char c[B];snprintf(c,B,"rm -rf '%s'",R[x-na].p);(void)!system(c);printf("\xe2\x9c\x93 %s\n",R[x-na].n);}
        return 0;}
    if(sel&&*sel>='0'&&*sel<='9'){int x=atoi(sel);
        if(x<na&&A[x].pid[0]){char c[B];snprintf(c,B,"tmux select-window -t '%s'",A[x].pid);(void)!system(c);tm_go(A[x].sn);}
        else if(x<na){perf_disarm();execlp("a","a","ssh",A[x].dev,"tmux","attach","-t",A[x].sn,(char*)NULL);/*foot terminal can fail here; ptyxis works*/}
        else if(x-na<nr){perf_disarm();if(chdir(R[x-na].p)==0){const char*sh=getenv("SHELL");execlp(sh?sh:"/bin/bash",sh?sh:"bash",(char*)NULL);}}
        return 0;}
    hub_load();
    if(!na&&!nr&&!NJ){puts("No jobs");return 0;}
    if(na){puts("ACTIVE");for(int i=0;i<na;i++)printf(" %d %-12s %-5s %-5s %s\n",i,A[i].sn,A[i].cmd,A[i].p,A[i].dev);}
    if(NJ){hub_sort();hub_timers();
        if(na)puts("");printf("SCHEDULED\n  %-10s %-6s %-8s  %s\n","Name","Sched","Dev","Cmd");for(int i=0;i<NJ;i++){
        hub_t*j=&HJ[i];char cp[128];hub_trunc(cp,128,j->p,50);
        printf("  %-10s %-6s %-8.7s%s %s\n",j->n,j->s,j->d,hub_on(j)?"\xe2\x9c\x93":" ",cp);}}
    if(na||NJ)puts("");printf("REVIEW\n  %-4s %-16s %s\n","#","Project","Branch");
    if(nr)for(int i=0;i<nr;i++){
        char*d=R[i].n,*s=strrchr(d,'-'),*s2=NULL;if(s){for(char*p=s-1;p>=d;p--)if(*p=='-'){s2=p;break;}}
        if(s2)printf("  %-4d%-16.*s %s\n",na+i,(int)(s2-d),d,s2+1);
        else printf("  %-4d%s\n",na+i,d);}
    else puts("  (none)");
    printf("\n  a j \"prompt\"  new job    a j a  agent    a job #  attach    a job rm #|all\n  a hub add <name> <sched> <cmd>  schedule recurring    a hub  manage\n  e %s/common/prompts/job.txt\n",SROOT);
    if(!nr||!isatty(STDIN_FILENO))return 0;
    for(int ri=0;ri>=0&&ri<nr;){
        printf("\n\033[1m\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81 %d/%d %s\033[0m\n",ri+1,nr,R[ri].n);
        {char c[B];snprintf(c,B,"cd '%s'&&a diff 2>/dev/null",R[ri].p);(void)!system(c);
        snprintf(c,B,"%s/.a_done",R[ri].p);char*d=readf(c,NULL);if(d){printf("%s\n",d);free(d);}}
        raw_enter();printf("\n  [m]erge [r]esume [d]el [j/k/q]  ");fflush(stdout);
        int k=raw_key();raw_exit();putchar('\n');char gd[B]="",c[B];
        if(k=='m'||k=='d'){snprintf(c,B,"git -C '%s' rev-parse --path-format=absolute --git-common-dir 2>/dev/null",R[ri].p);pcmd(c,gd,B);gd[strcspn(gd,"\n")]=0;{char*s=strrchr(gd,'/');if(s)*s=0;}}
        if(k=='m'&&gd[0]){
            snprintf(c,B,"cd '%s'&&git add -A&&(git diff --cached --quiet||git commit -m 'job: auto-commit')",R[ri].p);pcmd(c,NULL,0);
            char cc[B];snprintf(cc,B,"claude \"Merge j-%s into main. Resolve conflicts. Show diff --stat when done.\"",R[ri].n);
            if(!getenv("TMUX")){tm_new("merge",gd,cc);snprintf(c,B,"tmux split-window -v -p 50 -t merge -c '%s'",gd);(void)!system(c);tm_go("merge");}
            else{snprintf(c,B,"tmux new-window -n merge -c '%s' '%s'",gd,cc);(void)!system(c);
                snprintf(c,B,"tmux split-window -v -p 50 -c '%s'",gd);(void)!system(c);}}
        else if(k=='d'){snprintf(c,B,"rm -rf '%s'",R[ri].p);pcmd(c,NULL,0);
            if(gd[0]){snprintf(c,B,"(git -C '%s' worktree prune;git -C '%s' branch -D 'j-%s')>/dev/null 2>&1 &",gd,gd,R[ri].n);pcmd(c,NULL,0);}}
        else if(k=='r'){char jc[B];jcmd_fill(jc,1);
            if(!getenv("TMUX")){char sn[64];snprintf(sn,64,"j-%s",R[ri].n);tm_new(sn,R[ri].p,jc);tm_go(sn);}
            else{snprintf(c,B,"tmux new-window -n '%s' -c '%s' '%s'",R[ri].n,R[ri].p,jc);(void)!system(c);}}
        if(k=='d'){nr--;memmove(R+ri,R+ri+1,(size_t)(nr-ri)*sizeof(R[0]));if(ri>=nr)ri=nr-1;}
        else if(k=='k'){if(ri>0)ri--;}else if(k=='q'||k==3||k==27)break;else if(k=='j')ri++;
    }return 0;
}

/* ── tree ── */
static int cmd_tree(int argc, char **argv) { AB;
    init_db(); load_cfg(); load_proj();
    const char *wt = cfget("worktrees_dir"); if (!wt[0]) { char d[P]; snprintf(d,P,"%s/worktrees",AROOT); wt=d; }
    CWD(cwd);
    const char *proj = cwd;
    if (argc > 2 && argv[2][0]>='0' && argv[2][0]<='9') { int idx=atoi(argv[2]); if(idx<NPJ) proj=PJ[idx].path; }
    if (!git_in_repo(proj)) { puts("x Not a git repo"); return 1; }
    time_t now = time(NULL); struct tm *t = localtime(&now);
    char ts[32]; strftime(ts, 32, "%b%d", t);
    for(char*p=ts;*p;p++) *p=(*p>='A'&&*p<='Z')?*p+32:*p;
    int h=t->tm_hour%12; if(!h)h=12;
    char nm[64],wp[P],c[B];
    snprintf(nm,64,"%s-%s-%d%02d%s",bname(proj),ts,h,t->tm_min,t->tm_hour>=12?"pm":"am");
    for(int i=0;i<2;i++){if(i){size_t l=strlen(nm);char a[3]={nm[l-2],nm[l-1],0};sprintf(nm+l-2,"-%02d%s",t->tm_sec,a);}
        snprintf(wp,P,"%s/%s",wt,nm);snprintf(c,B,"mkdir -p '%s' && git -C '%s' worktree add -b 'wt-%s' '%s' HEAD 2>/dev/null",wt,proj,nm,wp);
        if(!system(c)){char sl[B];snprintf(sl,B,"ln -s '%s' '%s/adata' 2>/dev/null",AROOT,wp);(void)!system(sl);break;}
        if(i){puts("x Failed");return 1;}}
    printf("\xe2\x9c\x93 %s\n", wp);
    const char *sh = getenv("SHELL"); if (!sh) sh = "/bin/bash";
    if (chdir(wp) == 0) execlp(sh, sh, (char*)NULL);
    return 0;
}
