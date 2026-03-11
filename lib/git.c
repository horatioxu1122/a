/* ═══ GIT HELPERS ═══ */
static int git_in_repo(const char *p) {
    char c[P]; snprintf(c, P, "%s/.git", p); return dexists(c)||fexists(c);
}

/* ═══ ADATA SETUP ═══ */
static void ensure_adata(void) {
    char c[B],out[256];
    if(!git_in_repo(SROOT)){
        snprintf(c,B,"gh repo clone seanpattencode/a-git '%s' 2>/dev/null",SROOT);
        if(!system(c)){puts("\xe2\x9c\x93 Cloned adata/git");goto link;}
        mkdirp(SROOT);
        snprintf(c,B,"git -C '%s' init -q&&git -C '%s' checkout -b main 2>/dev/null",SROOT,SROOT);
        (void)!system(c);puts("\xe2\x9c\x93 Init adata/git (gh auth login for sync)");goto link;
    }
    snprintf(c,B,"git -C '%s' remote get-url origin 2>/dev/null",SROOT);
    pcmd(c,out,256);out[strcspn(out,"\n")]=0;
    if(!out[0]){char ghuser[64]="";
        pcmd("gh api user --jq .login 2>/dev/null",ghuser,sizeof(ghuser));ghuser[strcspn(ghuser,"\n")]=0;
        if(ghuser[0]){snprintf(c,B,"git -C '%s' remote add origin https://github.com/%s/a-git.git 2>/dev/null",SROOT,ghuser);
            (void)!system(c);printf("\xe2\x9c\x93 Added remote adata/git \xe2\x86\x92 %s/a-git\n",ghuser);}
    }
link:{char d[P];snprintf(d,P,"%s/my",SROOT);mkdir(d,0755);snprintf(d,P,"%s/my",SDIR);unlink(d);symlink("adata/git/my",d);}
}

static void ensure_git_id(void) {
    char n[128];pcmd("git config user.name 2>/dev/null",n,128);
    if(n[0]&&n[0]!='\n')return;
    pcmd("gh api user -q .login 2>/dev/null",n,128);n[strcspn(n,"\n")]=0;
    char e[128],c[B];
    if(n[0]){pcmd("gh api user -q .email 2>/dev/null",e,128);e[strcspn(e,"\n")]=0;
        if(!e[0]||!strcmp(e,"null"))snprintf(e,128,"%s@users.noreply.github.com",n);
    }else{gethostname(n,128);snprintf(e,128,"%s@local",n);}
    snprintf(c,B,"git config --global user.name '%s'&&git config --global user.email '%s'",n,e);
    (void)!system(c);printf("\xe2\x9c\x93 git id: %s <%s>\n",n,e);
}
/* ═══ SYNC — append-only, hub_save cleans old {name}_*.txt (see 2026-03-06 HSU incident) ═══ */
static void sync_repo(void) {
    ensure_git_id();
    char c[B];
    snprintf(c,B,"D='%s';rm -f $D/.git/index.lock $D/.git/refs/remotes/origin/*.lock;git -C $D add -A&&git -C $D commit -qm sync&&git -C $D pull --rebase -q origin main 2>/dev/null;git -C $D push -q origin main 2>/dev/null",SROOT);
    (void)!system(c);
}
static void sync_bg(void) {
    pid_t p=fork();if(p<0)return;if(p>0){waitpid(p,NULL,WNOHANG);return;}
    if(fork()>0)_exit(0);setsid();freopen("/dev/null","w",stdout);sync_repo();_exit(0);
}
