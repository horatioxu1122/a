/* help */
static const char *HELP_SHORT =
    "a j \"prompt\"     Job: worktree + agent\n"
    "a a|c|co|g|ai   Default/claude/codex/gemini/aider\n"
    "a <#>           Open project by number\n"
    "a help          All commands";

static const char *HELP_FULL =
    "a - AI agent session manager\n\n"
    "JOBS\n"
    "  a j \"prompt\"        Worktree + agent (a j <#> for project)\n"
    "  a job               Active jobs + review worktrees\n"
    "  a done \"msg\"        PR + email from worktree\n\n"
    "AGENTS        c=claude  co=codex  g=gemini  ai=aider\n"
    "  a a                 Default agent (a config default_agent <key>)\n"
    "  a <key>             Start in current dir (a <key> <#> for project)\n"
    "  a <key>++           Start in new worktree\n"
    "  a all               Multi-agent parallel runs\n"
    "  a agent             Agent menu + autonomous subagent\n\n"
    "PROJECTS\n"
    "  a <#>               cd to project #\n"
    "  a add / remove / move / scan\n"
    "  a create <name>     New repo\n\n"
    "GIT\n"
    "  a push [msg]        Commit and push\n"
    "  a pr [title]        Push branch + create PR\n"
    "  a pull / diff / revert\n\n"
    "NOTES & TASKS\n"
    "  a n \"text\"          Quick note\n"
    "  a task              Tasks (priority, review)\n\n"
    "REMOTE\n"
    "  a ssh [<#>]         List or connect to hosts\n"
    "  a run <#> \"task\"    Run task on remote\n\n"
    "SYSTEM\n"
    "  a ls / kill / attach   Tmux sessions\n"
    "  a hub               Scheduled jobs\n"
    "  a config / sync / update / perf";

/* list + cache */
static void list_all(int cache, int quiet) {
    load_proj(); load_apps();
    char pfile[P]; snprintf(pfile, P, "%s/projects.txt", DDIR);
    /* Write projects.txt for shell function */
    FILE *pf = fopen(pfile, "w");
    {int i; if (pf) { for (i = 0; i < NPJ; i++) fprintf(pf, "%s\n", PJ[i].path); fclose(pf); }}
    if (quiet && !cache) return;
    char out[B*4] = ""; int o = 0;
    if (NPJ) {
        o += sprintf(out + o, "PROJECTS:\n");
        int i; for (i = 0; i < NPJ; i++) {
            char mk = dexists(PJ[i].path) ? '+' : (PJ[i].repo[0] ? '~' : 'x');
            o += sprintf(out + o, "  %d. %c %s\n", i, mk, PJ[i].path);
        }
    }
    if (NAP) {
        o += sprintf(out + o, "COMMANDS:\n");
        int i; for (i = 0; i < NAP; i++) {
            char dc[64]; snprintf(dc, 64, "%s", AP[i].cmd);
            o += sprintf(out + o, "  %d. %s -> %s\n", NPJ + i, AP[i].name, dc);
        }
    }
    if (!quiet && out[0]) printf("%s", out);
    if (cache) {
        char cf[P]; snprintf(cf, P, "%s/help_cache.txt", DDIR);
        FILE *f = fopen(cf, "w");
        if (f) { fprintf(f, "%s\n%s", HELP_SHORT, out); fclose(f); }
        snprintf(cf, P, "%s/i_cache.txt", DDIR); unlink(cf);
    }
}

static void gen_icache(void) {
    load_proj(); load_apps(); load_cfg(); load_sess();
    char ic[P]; snprintf(ic, P, "%s/i_cache.txt", DDIR);
    FILE *f = fopen(ic, "w"); if (!f) return;
    fputs("a\tdefault agent\n",f);
    int i; for (i=0;i<NPJ;i++) fprintf(f, "%d: %s\tproject\n", i, PJ[i].name);
    for (i=0;i<NAP;i++) fprintf(f, "%d: %s\tcmd\n", NPJ+i, AP[i].name);
    for(i=0;i<NSE;i++)fprintf(f,"%s\t%s\n",SE[i].key,SE[i].name);
    {char ad[P];snprintf(ad,P,"%s/lab/platonic_agents",SDIR);DIR*d=opendir(ad);struct dirent*e;
    if(d){while((e=readdir(d))){char*p=strrchr(e->d_name,'.');
        if(p&&(p[1]=='p'||p[1]=='c')){*p=0;fprintf(f,"agent run %s\tagent\n",e->d_name);}}closedir(d);}}
    /* auto-discover lib .py — extract docstring desc */
    {char ld[P];snprintf(ld,P,"%s/lib",SDIR);DIR*d=opendir(ld);struct dirent*e;
    if(d){while((e=readdir(d))){char*dot=strrchr(e->d_name,'.');
        if(!dot||strcmp(dot,".py")||e->d_name[0]=='_')continue;*dot=0;
        char fp[P],desc[128]="";snprintf(fp,P,"%s/%s.py",ld,e->d_name);
        FILE*pf=fopen(fp,"r");if(pf){char h[256];if(fgets(h,256,pf)){
            char*s=strstr(h," - ");if(s){s+=3;s[strcspn(s,"\"\n")]=0;snprintf(desc,128,"%s",s);}}fclose(pf);}
        fprintf(f,"%s\t%s\n",e->d_name,desc[0]?desc:"cmd");}closedir(d);}}
    /* auto-discover my scripts */
    {char md[P];snprintf(md,P,"%s/my",SROOT);DIR*d=opendir(md);struct dirent*e;
    if(d){while((e=readdir(d))){if(e->d_name[0]=='.'||e->d_name[0]=='_')continue;
        char nm[64];snprintf(nm,64,"%s",e->d_name);char*dot=strrchr(nm,'.');if(dot)*dot=0;
        fprintf(f,"%s\tmy\n",nm);}closedir(d);}}
    /* auto-discover lab scripts */
    {char ld[P];snprintf(ld,P,"%s/lab",SDIR);DIR*d=opendir(ld);struct dirent*e;
    if(d){while((e=readdir(d))){char*dot=strrchr(e->d_name,'.');
        if(!dot||e->d_name[0]=='_')continue;
        if(strcmp(dot,".py")&&strcmp(dot,".c")&&strcmp(dot,".sh")&&strcmp(dot,".html"))continue;*dot=0;
        fprintf(f,"x.%s\tlab\n",e->d_name);}closedir(d);}}
    /* subcommands not discoverable from filenames */
    fputs("cal add\tadd event\nhub add\tadd\nhub run\trun\nhub rm\trm\nhub log\tlog\n"
    "note l\tlist\nnote r\treview\nssh add\tadd host\nssh all\tall hosts\n"
    "task add\tadd\ntask l\tlist\ntask r\treview\ntask rank\trank\n",f);
    char sd[P]; snprintf(sd, P, "%s/ssh", SROOT);
    char sp[32][P]; int sn = listdir(sd, sp, 32);
    for (i=0;i<sn;i++) {
        kvs_t kv = kvfile(sp[i]);
        const char *nm = kvget(&kv,"Name"); if (!nm) continue;  fprintf(f, "ssh %s\thost\n", nm);
    }
    fclose(f);
}

static int cmd_help(int argc, char **argv) { (void)argc; (void)argv;
    char p[P]; snprintf(p, P, "%s/help_cache.txt", DDIR);
    if (catf(p) < 0) { init_db(); load_cfg(); printf("%s\n", HELP_SHORT); list_all(1, 0); }
    return 0;
}

static int cmd_help_full(int argc, char **argv) { (void)argc; (void)argv;
    init_db(); load_cfg(); printf("%s\n", HELP_FULL); list_all(1, 0); return 0;
}

static int cmd_hi(int argc, char **argv) { (void)argc;(void)argv; for (int i = 1; i <= 10; i++) printf("%d\n", i); puts("hi"); return 0; }

static int cmd_done(int argc, char **argv) { AB;
    char p[P]; snprintf(p, P, "%s/.done", DDIR);
    char msg[B]="";ajoin(msg,B,argc,argv,2);
    FILE *f=fopen(p,"w");if(f){fputs(msg,f);fclose(f);}
    /* auto-PR if in worktree */
    char wd[P],gc[B],gd[P];if(!getcwd(wd,P))wd[0]=0;
    snprintf(gc,B,"git -C '%s' rev-parse --git-dir 2>/dev/null",wd);
    pcmd(gc,gd,P);gd[strcspn(gd,"\n")]=0;
    if(strstr(gd,"worktrees")){perf_disarm();
        char sm[B];snprintf(sm,B,"job: %.200s",msg[0]?msg:"done");
        snprintf(gc,B,"cd '%s'&&git add -A&&git diff --cached --quiet 2>/dev/null||"
            "(git commit -m '%s'&&git push -u origin HEAD 2>/dev/null&&"
            "gh pr create --fill 2>/dev/null)",wd,sm);
        int r=system(gc);
        if(!r)puts("+ PR created");
        /* email */
        char ad[B]="";snprintf(gc,B,"cd '%s'&&a diff main 2>/dev/null",wd);
        pcmd(gc,ad,B);
        snprintf(gc,B,"a email '[a job] %s' '%s\n%s'",bname(wd),msg[0]?msg:"done",ad);
        (void)!system(gc);
    }
    puts("\xe2\x9c\x93 done"); return 0;
}

static int cmd_dir(int argc, char **argv) { (void)argc;(void)argv;
    char cwd[P]; if (getcwd(cwd, P)) puts(cwd);
    execlp("ls", "ls", (char*)NULL); return 1;
}


static int cmd_x(int argc, char **argv) { (void)argc;(void)argv;
    (void)!system("tmux kill-server 2>/dev/null");
    puts("\xe2\x9c\x93 All sessions killed"); return 0;
}

static int cmd_web(int argc, char **argv) { AB;
    char url[B] = "https://google.com";
    if (argc > 2) {
        int l=snprintf(url,B,"https://google.com/search?q=");
        for(int i=2;i<argc&&l<B-1;i++) l+=snprintf(url+l,(size_t)(B-l),"%s%s",i>2?"+":"",argv[i]);
    }
    char c[B]; snprintf(c, B, "xdg-open '%s' 2>/dev/null &", url); (void)!system(c);
    return 0;
}


