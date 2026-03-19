/* ── session (c, l, g, co, cp, etc.) ── */
static int cmd_sess(int argc, char **argv) {
    init_db(); load_cfg(); load_proj(); load_apps(); load_sess(); tm_ensure_conf();
    const char *key = argv[1];
    sess_t *s = find_sess(key);
    if (!s) return -1;  /* not a session key */
    CWD(wd);
    const char *wda = argc > 2 ? argv[2] : NULL;
    /* If wda is a project number */
    if (wda && wda[0] >= '0' && wda[0] <= '9') {
        int idx = atoi(wda);
        if (idx >= 0 && idx < NPJ) snprintf(wd, P, "%s", PJ[idx].path);
        else if (idx >= NPJ && idx < NPJ + NAP) {
            printf("> Running: %s\n", AP[idx-NPJ].name);
            const char *sh = getenv("SHELL"); if (!sh) sh = "/bin/bash";
            execlp(sh, sh, "-c", AP[idx-NPJ].cmd, (char*)NULL);
        }
    } else if (wda && dexists(wda)) {
        if (wda[0] == '~') snprintf(wd, P, "%s%s", HOME, wda+1);
        else snprintf(wd, P, "%s", wda);
    }
    /* Build prompt from remaining args */
    char prompt[B]=""; int is_prompt=0,pl=0;
    int start = wda ? 3 : 2;
    if (wda && !(wda[0]>='0'&&wda[0]<='9') && !dexists(wda)) { start = 2; is_prompt = 1; }
    for (int i = start; i < argc; i++) {
        if (!strcmp(argv[i],"-w")||!strcmp(argv[i],"--new-window")||!strcmp(argv[i],"-t")||!strcmp(argv[i],"--with-terminal")) continue;
        pl+=snprintf(prompt+pl,(size_t)(B-pl),"%s%s",pl?" ":"",argv[i]);
        is_prompt = 1;
    }
    char sn[256]; snprintf(sn, 256, "%s-%s", s->name, bname(wd));
    /* claim ghost if matches */
    {char gf[P];snprintf(gf,P,"%s/ghost",DDIR);char*gh=readf(gf,NULL);
    if(gh){gh[strcspn(gh,"\n")]=0;if(!strcmp(gh,sn)&&tm_has(sn)){unlink(gf);free(gh);
        if(is_prompt&&prompt[0]){tm_send(sn,prompt);usleep(100000);tm_key(sn,"Enter");}
        tm_go(sn);return 0;}free(gh);}}
    /* Inside tmux = split pane mode (user wants agent HERE, not session switch) */
    if (getenv("TMUX") && strlen(key) == 1 && key[0] != 'a') {
        perf_disarm();
        char ww[16],nc[16]; pcmd("tmux display-message -p '#{window_width}'",ww,16);
        pcmd("tmux list-panes -F '#{pane_left}'|sort -un|wc -l",nc,16);
        int nw=atoi(nc)>0?atoi(ww)/(atoi(nc)+1):atoi(ww)/2;
        char c[B]; snprintf(c, B, "tmux split-window -hfP -l %d -F '#{pane_id}' -c '%s' 'unset CLAUDECODE CLAUDE_CODE_ENTRYPOINT; %s'", nw, wd, s->cmd);
        char pid[64]; pcmd(c, pid, 64); pid[strcspn(pid,"\n")] = 0;
        if (pid[0]) {
            snprintf(c, B, "tmux split-window -v -t '%s' -c '%s' 'sh -c \"ls;exec $SHELL\"'", pid, wd); (void)!system(c);
            snprintf(c, B, "tmux select-pane -t '%s'", pid); (void)!system(c);
            send_prefix_bg(pid, s->name, wd, is_prompt ? prompt : NULL);
        }
        return 0;
    }
    /* Existing session = attach (outside tmux only) */
    if (tm_has(sn)) {
        if (is_prompt && prompt[0]) {
            tm_send(sn, prompt); usleep(100000);
            tm_key(sn, "Enter");
            puts("Prompt queued (existing session)");
        }
        tm_go(sn);
        return 0;
    }
    create_sess(sn, wd, s->cmd);
    send_prefix_bg(sn, s->name, wd, is_prompt ? prompt : NULL);
    tm_go(sn);
    return 0;
}

/* ── worktree ++ ── */
static int cmd_wt_plus(int argc, char **argv) { fallback_py("wt_plus", argc, argv); }

/* ── worktree w* ── */
static int cmd_wt(int argc, char **argv) { fallback_py("wt", argc, argv); }

/* ── dir_file ── */
static int cmd_dir_file(int argc, char **argv) { (void)argc;
    const char *arg = argv[1];
    char expanded[P];
    if (arg[0] == '~') snprintf(expanded, P, "%s%s", HOME, arg+1);
    else snprintf(expanded, P, "%s", arg);
    if (!dexists(expanded)&&!fexists(expanded)&&arg[0]=='/') snprintf(expanded,P,"%s%s",HOME,arg);
    if (dexists(expanded)) { printf("%s\n", expanded); execlp("ls", "ls", expanded, (char*)NULL); }
    else if (fexists(expanded)) {
        const char *ext = strrchr(expanded, '.');
        if (ext && !strcmp(ext, ".py")) {
            char py[P]="python3"; const char *ve=getenv("VIRTUAL_ENV");
            if(ve) snprintf(py,P,"%s/bin/python",ve);
            else if(!access(".venv/bin/python",X_OK)) snprintf(py,P,".venv/bin/python");
            execvp(py, (char*[]){ py, expanded, NULL });
        }
        else{int t=ext?ext[1]:0;const char*ed=t=='c'||t=='s'?"sh":t=='h'?"xdg-open":getenv("EDITOR");
            if(!ed)ed="e";execlp(ed,ed,expanded,(char*)NULL);}
    }
    return 0;
}

/* ── interactive picker ── */
typedef struct{char*p;int sc;}fqm_t;
static FC fq[256]; int nfq;
static int fq_get(const char*s){int sl=(int)strlen(s);
    for(int i=0;i<nfq;i++){int fl=(int)strlen(fq[i].n);if(fl<=sl&&!strncasecmp(s,fq[i].n,(size_t)fl))return fq[i].c;}return 0;}
static int fqm_cmp(const void*a,const void*b){return((const fqm_t*)b)->sc-((const fqm_t*)a)->sc;}
static int cmd_i(int argc, char **argv) { (void)argc; (void)argv;
    perf_disarm(); init_db();
    char cache[P]; snprintf(cache, P, "%s/i_cache.txt", DDIR);
    gen_icache();
    /* load freq cache */
    {char fp[P];snprintf(fp,P,"%s/freq_cache.txt",DDIR);
    FILE*ff=fopen(fp,"r");if(ff){char ln[128];nfq=0;
        while(nfq<256&&fgets(ln,128,ff)){char*c=strchr(ln,':');if(!c)continue;*c=0;
            snprintf(fq[nfq].n,64,"%s",ln);fq[nfq].c=atoi(c+1);nfq++;}fclose(ff);}}
    size_t len; char *raw = readf(cache, &len);
    if (!raw) { puts("No cache"); return 1; }
    /* Parse lines */
    char *lines[512]; int n = 0;
    for (char *p = raw, *end = raw + len; p < end && n < 512;) {
        char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl) nl = end;
        if (nl > p && p[0] != '<' && p[0] != '=' && p[0] != '>' && p[0] != '#') { *nl = 0; lines[n++] = p; }
        p = nl + 1;
    }
    if (!n) { puts("Empty cache"); free(raw); return 1; }
    if (!isatty(STDIN_FILENO)) { for (int i=0;i<n;i++) puts(lines[i]); free(raw); return 0; }
    /* Terminal size */
    struct winsize ws; ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int maxshow = ws.ws_row > 6 ? ws.ws_row - 3 : 10;
    /* Raw mode */
    struct termios old, raw_t;
    tcgetattr(STDIN_FILENO, &old); raw_t = old;
    raw_t.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG);
    raw_t.c_cc[VMIN] = 1; raw_t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_t);
    write(STDOUT_FILENO,"\033[?1000h\033[?1006h",16);
    char buf[256] = ""; int blen = 0, sel = 0; char prefix[256] = "";
    printf("Filter (↑↓/Tab=cycle, Enter=run, Esc=quit)\n");
    while (1) {
        /* Search */
        fqm_t fm[512]; int nm = 0; int plen = (int)strlen(prefix);
        for (int i=0;i<n&&nm<512;i++) {
            if (plen && strncmp(lines[i], prefix, (size_t)plen)) continue;
            if(blen){char*s=lines[i]+plen,b2[256],*w;snprintf(b2,256,"%s",buf);int ok=1;
                for(w=strtok(b2," ");w&&ok;w=strtok(0," "))if(!strcasestr(s,w))ok=0;if(!ok)continue;}
            fm[nm].p=lines[i];fm[nm].sc=blen?fq_get(lines[i]):0;nm++;
        }
        if(blen&&nfq)qsort(fm,(size_t)nm,sizeof(fqm_t),fqm_cmp);
        if (sel >= nm) sel = nm ? nm-1 : 0;
        /* Scroll window around sel */
        int top=sel>=maxshow?sel-maxshow+1:0, show=nm-top<maxshow?nm-top:maxshow;
        /* Render — single write to avoid flicker */
        {char fb[B*4];int fl=0;
        fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[H\033[?25l%s> %s\033[K\n",prefix,buf);
        for(int i=0;i<show;i++){int j=top+i,W=ws.ws_col;char*t=strchr(fm[j].p,'\t');int ml=t?(int)(t-fm[j].p):(int)strlen(fm[j].p);
            if(ml>W-5)ml=W-5;fl+=snprintf(fb+fl,(size_t)(B*4-fl),"%s a %.*s\033[K",j==sel?" >":"  ",ml,fm[j].p);
            if(t&&ml+5+(int)strlen(t+1)<W)fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%dG\033[90m%s\033[0m",W-(int)strlen(t+1),t+1);
            fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\n");}
        fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[J\033[1;%dH\033[?25h",plen+blen+3);
        (void)!write(STDOUT_FILENO,fb,(size_t)fl);}
        char ch; if(read(0,&ch,1)!=1) break;
        int do_pick=0;
        if(ch=='\x1b'){int av;usleep(50000);ioctl(0,FIONREAD,&av);if(!av)break;
            char seq[2];if(read(0,seq,1)!=1)break;
            if(seq[0]=='['){if(read(0,seq+1,1)!=1)break;
                if(seq[1]=='A'){if(sel>0)sel--;}
                else if(seq[1]=='B'){if(sel<nm-1)sel++;}
                else if(seq[1]=='<'){int mb=0,mx=0,my=0;char mc;
                    while(read(0,&mc,1)==1&&mc!=';')mb=mb*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!=';')mx=mx*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!='M'&&mc!='m')my=my*10+mc-'0';
                    (void)mx;if(mc=='M'&&mb==0){int ci=my-2+top;if(ci>=0&&ci<nm){sel=ci;do_pick=1;}}
                    else if(mc=='M'&&(mb==64||mb==65)){if(mb==64&&sel>0)sel--;if(mb==65&&sel<nm-1)sel++;}}
            } else if(prefix[0]){prefix[0]=0;buf[0]=0;blen=0;sel=0;} else break;
        } else if (ch == '\t') { if(sel<nm-1)sel++; }
        else if (ch == '\x7f' || ch == '\b') { if (blen) buf[--blen]=0; sel=0; }
        else if (ch == '\r' || ch == '\n') { do_pick=1; }
        else if (ch == '\x03' || ch == '\x04') break;
        else if ((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch==' ') { if(blen<254){buf[blen++]=ch;buf[blen]=0;sel=0;} }
        if (do_pick && nm) {
            char *m = fm[sel].p; char cmd[256];
            char *tab=strchr(m,'\t'),*colon=strchr(m,':');
            if(colon&&(!tab||colon<tab)){int cl=(int)(colon-m);snprintf(cmd,256,"%.*s",cl,m);while(cmd[0]==' ')memmove(cmd,cmd+1,strlen(cmd));}
            else{int cl=tab?(int)(tab-m):(int)strlen(m);snprintf(cmd,256,"%.*s",cl,m);}
            char *e = cmd+strlen(cmd)-1; while(e>cmd&&*e==' ')*e--=0;
            int has_sub=0,cl=(int)strlen(cmd);
            for(int i=0;i<n;i++) if(!strncmp(lines[i],cmd,(size_t)cl)&&lines[i][cl]==' '){has_sub=1;break;}
            if(has_sub){snprintf(prefix,256,"%s ",cmd);buf[0]=0;blen=0;sel=0;printf("\033[J");continue;}
            tcsetattr(STDIN_FILENO, TCSANOW, &old);
            write(STDOUT_FILENO,"\033[?1000l\033[?1006l",16);
            (void)!system("clear");
            printf("Running: a %s\n", cmd);
            char *args[32]; int ac=0; args[ac++]="a";
            char *p=cmd; while(*p&&ac<31) { while(*p==' ')p++; if(!*p)break; args[ac++]=p; while(*p&&*p!=' ')p++; if(*p)*p++=0; }
            args[ac]=NULL;
            free(raw); execvp("a", args);
            return 0;
        }
    }
    write(STDOUT_FILENO,"\033[?1000l\033[?1006l",16);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    printf("\033[2J\033[H"); free(raw);
    return 0;
}
