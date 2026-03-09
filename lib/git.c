/* ═══ GIT HELPERS ═══ */
static int git_in_repo(const char *p) {
    char c[P]; snprintf(c, P, "%s/.git", p); return dexists(c)||fexists(c);
}

/* ═══ ADATA SETUP ═══ */
static void ensure_adata(void) {
    char c[B], out[512];
    if (!git_in_repo(SROOT)) {
        snprintf(c, B, "command -v gh >/dev/null 2>&1 && gh repo clone seanpattencode/a-git '%s' 2>/dev/null", SROOT);
        if (system(c) == 0) { printf("\xe2\x9c\x93 Cloned adata/git\n"); return; }
        mkdirp(SROOT);
        snprintf(c, B, "git -C '%s' init -q 2>/dev/null && git -C '%s' checkout -b main 2>/dev/null", SROOT, SROOT);
        (void)!system(c);
        printf("\xe2\x9c\x93 Initialized adata/git (gh auth login to enable sync)\n");
        return;
    }
    snprintf(c, B, "git -C '%s' remote get-url origin 2>/dev/null", SROOT);
    pcmd(c, out, sizeof(out)); out[strcspn(out, "\n")] = 0;
    if (!out[0]) {
        char ghuser[64]="";
        pcmd("gh api user --jq .login 2>/dev/null", ghuser, sizeof(ghuser));
        ghuser[strcspn(ghuser,"\n")]=0;
        if (ghuser[0]) {
            snprintf(c,B,"git -C '%s' remote add origin https://github.com/%s/a-git.git 2>/dev/null",SROOT,ghuser);
            (void)!system(c);
            printf("\xe2\x9c\x93 Added remote adata/git \xe2\x86\x92 github.com/%s/a-git\n",ghuser);
        }
    }
}

/* ═══ SYNC — append-only, hub_save cleans old {name}_*.txt (see 2026-03-06 HSU incident) ═══ */
static void ensure_git_identity(void) {
    char name[128]="", email[128]="";
    pcmd("git config --global user.name 2>/dev/null", name, sizeof(name));
    pcmd("git config --global user.email 2>/dev/null", email, sizeof(email));
    name[strcspn(name,"\n")]=0; email[strcspn(email,"\n")]=0;
    if (name[0] && email[0]) return;
    printf("Git identity not set. Enter your name: "); fflush(stdout);
    if (!name[0]) {
        if (!fgets(name, sizeof(name), stdin)) return;
        name[strcspn(name,"\n")]=0;
        char c[B]; snprintf(c,B,"git config --global user.name '%s'",name); (void)!system(c);
    }
    if (!email[0]) {
        printf("Enter your email: "); fflush(stdout);
        if (!fgets(email, sizeof(email), stdin)) return;
        email[strcspn(email,"\n")]=0;
        char c[B]; snprintf(c,B,"git config --global user.email '%s'",email); (void)!system(c);
    }
    printf("\xe2\x9c\x93 Git identity set\n");
}
static void sync_repo(void) {
    ensure_git_identity();
    char c[B], remote[256]="";
    snprintf(c,B,"git -C '%s' remote get-url origin 2>/dev/null",SROOT);
    pcmd(c,remote,sizeof(remote)); remote[strcspn(remote,"\n")]=0;
    int has_remote = 0;
    if (remote[0]) {
        char chk[B]; snprintf(chk,B,"git -C '%s' ls-remote --exit-code origin HEAD >/dev/null 2>&1",SROOT);
        has_remote = system(chk)==0;
    }
    snprintf(c,B,"D='%s';rm -f $D/.git/index.lock;git -C $D add -A&&git -C $D commit -qm sync%s",SROOT,
        has_remote?";git -C $D pull --no-rebase --no-edit -q origin main;git -C $D push -q origin main":"");
    (void)!system(c);
}
static void sync_bg(void) {
    pid_t p=fork();if(p<0)return;if(p>0){waitpid(p,NULL,WNOHANG);return;}
    if(fork()>0)_exit(0);setsid();freopen("/dev/null","w",stdout);freopen("/dev/null","w",stderr);sync_repo();_exit(0);
}
