/* fallback py */
__attribute__((noreturn))
static void fallback_py(const char *mod, int argc, char **argv) {
    if (getenv("A_BENCH")) _exit(0);
    perf_disarm(); setenv("PYTHONDONTWRITEBYTECODE","1",1);
    char path[P]; snprintf(path, P, "%s/lib/%s.py", SDIR, mod);
    char **a = malloc(((unsigned)argc + 5) * sizeof(char *));
    {FILE *f=fopen(path,"r");char h[32]={0};
    if(f){(void)!fgets(h,32,f);fclose(f);}
    if(strstr(h,"/// script")){
        char uv[P];snprintf(uv,P,"%s/.local/bin/uv",HOME);
        a[0]="uv";a[1]="run";a[2]="--script";a[3]=path;
        for(int i=1;i<argc;i++)a[i+3]=argv[i];a[argc+3]=NULL;
        if(access(uv,X_OK)==0){a[0]=uv;execv(uv,a);}
        execvp("uv",a);}}
    a[0]="python3";a[1]=path;
    for(int i=1;i<argc;i++)a[i+1]=argv[i];a[argc+1]=NULL;
    execvp("python3",a);
    perror("a: python3");_exit(127);
}

/* session create — returns 1 if window already existed (restored), 0 if created */
static int create_sess(const char *sn, const char *wd, const char *cmd) {
    int ai = cmd && (strstr(cmd,"claude") || strstr(cmd,"codex") || strstr(cmd,"gemini") || strstr(cmd,"aider"));
    char acmd[B];snprintf(acmd,B,"%s",cmd?cmd:"");
    if(ai&&in_fork(wd)){const char*fk=strstr(wd,"/adata/forks/")+13;snprintf(acmd,B,"a fork run %s %s",fk,cmd);}
    char wcmd[B*2],ctxf[P]="",csuf[256]="";
    if(ai&&strstr(acmd,"claude")){snprintf(ctxf,P,"/tmp/a_ctx_%d.txt",(int)getpid());snprintf(csuf,256," --append-system-prompt-file %s",ctxf);}
    if (ai) snprintf(wcmd, sizeof(wcmd),
        "unset CLAUDECODE CLAUDE_CODE_ENTRYPOINT;%s%s%swhile :;do %s%s;e=$?;[ $e -eq 0 ]&&break;echo \"$(date) $e $(pwd)\">>%s/crashes.log;echo -e \"\\n! crash $e [R]estart/[Q]uit:\";read -n1 k;[[ $k =~ [Rr] ]]||break;done", ctxf[0]?"a cat 1 >":"",ctxf,ctxf[0]?" 2>/dev/null;":"",acmd,csuf,LOGDIR);
    else snprintf(wcmd, sizeof(wcmd), "%s", cmd ? cmd : "");
    tm_ensure_conf();
    int r = tm_new(sn, wd, wcmd);
    if (!r) {
        if (ai) {
            char c[B]; snprintf(c, B, "tmux split-window -v -t '%s:%s' -c '%s' 'sh -c \"ls;exec $SHELL\"'", TMS, sn, wd);
            (void)!system(c);
            snprintf(c, B, "tmux select-pane -t '%s:%s' -U", TMS, sn); (void)!system(c);
        }
        /* logging */
        mkdirp(LOGDIR); char c[B];
        char lf[P]; snprintf(lf, P, "%s/%s__%s.log", LOGDIR, DEV, sn);
        snprintf(c, B, "tmux pipe-pane -t '%s:%s' 'cat >> %s'", TMS, sn, lf); (void)!system(c);
        char al[B]; snprintf(al, B, "session:%s log:%s", sn, lf);
        alog(al, wd);
        char alf[P]; snprintf(alf, P, "%s/agent_logs.txt", DDIR);
        time_t now = time(NULL);
        FILE *af = fopen(alf, "a"); if (af) { fprintf(af, "%s %ld %s\n", sn, (long)now, DEV); fclose(af); }
    }
    tm_save_win(sn, wd);
    return r;
}

static void send_prefix_bg(const char *sn, const char *agent, const char *wd, const char *extra) {
    const char *cp = strstr(agent, "claude") ? cfget("claude_prefix") : "";
    char pre[B*4]; int n = snprintf(pre, sizeof(pre), "%s%s", dprompt(), cp);
    n += snprintf(pre+n, sizeof(pre)-(unsigned)n,
        " When work finished, run the a done command with a message to notify human. In multi turn conversations run a done each time work finished."
	" a agent manager tools: "
	" a done <Message>. - tmux bell red dot notification."
        " a help -� command list."
        " a diff — tok change vs main."
        " a note <text> — l note."
        " a cat 2 — rd whole codebase."
	" a cat 3 - read project root and first and last parts of other files"
	" a ssh - ssh to devices.\n");
    char af[P]; snprintf(af, P, "%s/AGENTS.md", wd);
    char *amd = readf(af, NULL);
    if (amd) { n += snprintf(pre+n, sizeof(pre)-(unsigned)n, "%s ", amd); free(amd); }
    if (extra) snprintf(pre+n, sizeof(pre)-(unsigned)n, "%s", extra);
    if (!pre[0]) return;
    if (fork() == 0) {
        setsid();
        for (int i = 0; i < 300; i++) {
            usleep(50000);
            char buf[B] = "";
            tm_read(sn, buf, B);
            char *lo = buf;
            for (char *p = lo; *p; p++) *p = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
            if (strstr(lo,"context") || strstr(lo,"claude") || strstr(lo,"opus") || strstr(lo,"shortcut") || strstr(lo,"codex")) break;
        }
        tm_send(sn, pre);
        if (extra) { sleep(1); tm_key(sn, "Enter"); }
        _exit(0);
    }
}

static void tm_restore(void) {
    char sf[P];snprintf(sf,P,"%s/tmux_wins.txt",DDIR);
    char*d=readf(sf,NULL);if(!d)return;
    if(!NSE){init_db();load_cfg();load_sess();}
    for(char*l=d,*nl;*l;l=nl?nl+1:l+strlen(l)){
        nl=strchr(l,'\n');if(nl)*nl=0;
        char*s=strchr(l,'|');if(!s)continue;*s=0;
        if(!dexists(s+1))continue;
        char key[16]="",*dash=strchr(l,'-');
        if(dash)snprintf(key,16,"%.*s",(int)(dash-l),l);
        sess_t*se=find_sess(key);if(!se)continue;
        char cmd[1024];
        int resume=strstr(se->cmd,"claude")||strstr(se->cmd,"gemini");
        snprintf(cmd,1024,resume?"%s --continue":"%s",se->cmd);
        create_sess(l,s+1,cmd);}
    free(d);}

static void tm_unsave_win(const char *sn) {
    char sf[P];snprintf(sf,P,"%s/tmux_wins.txt",DDIR);
    char*d=readf(sf,NULL);if(!d)return;
    int sl=(int)strlen(sn);FILE*f=fopen(sf,"w");if(!f){free(d);return;}
    for(char*l=d;*l;){char*nl=strchr(l,'\n');int ll=nl?(int)(nl-l):(int)strlen(l);
        if(ll>0&&!(ll>=sl&&l[sl]=='|'&&!memcmp(l,sn,(size_t)sl)))fprintf(f,"%.*s\n",ll,l);
        l=nl?nl+1:l+ll;}
    free(d);fclose(f);}

static int cmd_restore(int c,char**v){(void)c;(void)v;
    init_db();load_cfg();load_sess();tm_restore();return 0;}
