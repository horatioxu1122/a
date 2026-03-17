/* init */
static void init_paths(void) {
    const char *h = getenv("HOME"); if (!h) h = "/tmp";
    snprintf(HOME, P, "%s", h);
    /* Projects live in ~ — wherever the default terminal opens.
     * a is just a folder there. Projects are just folders next to it. */
    {struct stat ps; char op[P],mc[B];
     const char*od[]={"p","projects",NULL};
     for(int i=0;od[i];i++){
         snprintf(op,P,"%s/%s",h,od[i]);
         if(lstat(op,&ps)==0&&S_ISLNK(ps.st_mode)){unlink(op);continue;}
         if(!lstat(op,&ps)&&S_ISDIR(ps.st_mode)){
             snprintf(mc,B,"mv -n '%s/'* '%s/' 2>/dev/null;rm -rf '%s';sed -i 's|^_ADD=.*|_ADD=\"%s/a/adata/local\"|' '%s/.bashrc' '%s/.zshrc' 2>/dev/null",op,h,op,h,h,h);
             (void)!system(mc);
             /* fix symlinks that pointed into the old ~/projects/ or ~/p/ dir */
             snprintf(mc,B,"find '%s' -maxdepth 3 -type l 2>/dev/null|while read l;do t=$(readlink \"$l\");case \"$t\" in */%s/*)ln -sfn \"%s/${t#*/%s/}\" \"$l\";;esac;done",h,od[i],h,od[i]);
             (void)!system(mc);
             snprintf(mc,B,"%s/.local/bin/a",h);unlink(mc);
             snprintf(op,P,"%s/a/adata/local/a",h);symlink(op,mc);
             fprintf(stderr,"! migrated ~/%s → ~/\n",od[i]);}}}
    {const char*t=getenv("TMPDIR");snprintf(TMP,P,"%s",t&&*t?t:"/tmp");}
    char self[P]; ssize_t n = -1;
#ifdef __APPLE__
    uint32_t sz = P - 1;
    if (_NSGetExecutablePath(self, &sz) == 0) { n = (ssize_t)strlen(self); char rp[P]; if (realpath(self, rp)) { snprintf(self, P, "%s", rp); n = (ssize_t)strlen(self); } }
#else
    n = readlink("/proc/self/exe", self, P - 1);
#endif
    if (n > 0) {
        self[n] = 0;
        char *s = strrchr(self, '/');
        if (s) { *s = 0;
            /* binary lives in adata/local/ — strip to project root */
            {char *al=strstr(self,"/adata/local");if(al)*al=0;}
            {char *wt=strstr(self,"/adata/worktrees/");if(wt)*wt=0;}
            snprintf(SDIR, P, "%s", self);
            snprintf(AROOT, P, "%s/adata", self);
            snprintf(SROOT, P, "%s/git", AROOT);
        }
    }
    if (!SROOT[0]) { snprintf(AROOT, P, "%s/a/adata", h); snprintf(SROOT, P, "%s/git", AROOT); }
    /* All local state lives in adata/ — if it's not in adata, nobody knows
     * where it is. Maximum visibility for humans and LLMs. */
    snprintf(DDIR, P, "%s/local", AROOT);
    mkdirp(DDIR);
    /* One-time migration: old sibling adata/ → inside project dir */
    char old_sib[P]; snprintf(old_sib, P, "%.*s/adata", (int)(strlen(SDIR) - strlen("/a")), SDIR);
    /* only migrate if old sibling exists and new doesn't have .device yet */
    char new_dev[P]; snprintf(new_dev, P, "%s/.device", DDIR);
    struct stat mst;
    if (strcmp(old_sib, AROOT) != 0 && stat(old_sib, &mst) == 0 && (stat(new_dev, &mst) != 0 || stat(SROOT, &mst) != 0)) {
        char mc[B]; snprintf(mc, B, "find '%s' -xtype l -delete 2>/dev/null;cp -rn '%s/'* '%s/' 2>/dev/null;find '%s' -xtype l -delete 2>/dev/null", AROOT, old_sib, AROOT, AROOT);
        (void)!system(mc);
    }
    /* Also migrate from old ~/.local/share/a/ */
    char old_local[P]; snprintf(old_local, P, "%s/.local/share/a/.device", h);
    if (stat(old_local, &mst) == 0 && stat(new_dev, &mst) != 0) {
        char mc[B]; snprintf(mc, B, "cp -rn '%s/.local/share/a/'* '%s/' 2>/dev/null", h, DDIR);
        (void)!system(mc);
    }
    /* device id */
    char df[P]; snprintf(df, P, "%s/.device", DDIR);
    FILE *f = fopen(df, "r");
    if (f) { if (fgets(DEV, 128, f)) DEV[strcspn(DEV, "\n")] = 0; fclose(f); }
    if (!DEV[0]) {
        gethostname(DEV, 128);
        mkdirp(DDIR);
        f = fopen(df, "w"); if (f) { fputs(DEV, f); fclose(f); }
    }
    snprintf(LOGDIR, P, "%s/backup/%s", AROOT, DEV);
    {char rm[P];snprintf(rm,P,"%s/README",AROOT);struct stat st;
    if(stat(rm,&st)!=0){f=fopen(rm,"w");if(f){fputs("adata/ - 4-tier data sync\n\n"
        "  git/    push/pull   sync/   rclone   vault/  on-demand   backup/ upload+purge\n",f);fclose(f);}}}
}
