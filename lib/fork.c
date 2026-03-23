/* fork — independent local copies, git remote intact for direct push */
static int in_fork(const char *p) { return strstr(p, "/adata/forks/") != NULL; }

static int cmd_fork(int argc, char **argv) {
    char fd[P]; snprintf(fd,P,"%s/forks",AROOT); mkdirp(fd);
    const char *sub=argc>2?argv[2]:NULL;

    if(!sub||!strcmp(sub,"ls")) {
        char c[B]; snprintf(c,B,"ls -d '%s'/*/.git 2>/dev/null",fd);
        FILE*f=popen(c,"r"); char ln[P]; int n=0;
        if(f){while(fgets(ln,P,f)){ln[strcspn(ln,"\n")]=0;
            char*s=strrchr(ln,'/');if(s)*s=0;s=strrchr(ln,'/');
            char st[128]="";{char gc[B];snprintf(gc,B,"git -C '%s' status --short 2>/dev/null|wc -l",ln);pcmd(gc,st,128);}
            int d=atoi(st);printf("  %s%s\n",s?s+1:"?",d?" *":"");n++;}pclose(f);}
        if(!n)puts("no forks");
        return 0;
    }

    if(!strcmp(sub,"rm")) {
        if(argc<4){fprintf(stderr,"Usage: a fork rm <name>\n");return 1;}
        char fp[P]; snprintf(fp,P,"%s/%s",fd,argv[3]);
        struct stat st; if(stat(fp,&st)){fprintf(stderr,"x %s?\n",argv[3]);return 1;}
        char c[B]; snprintf(c,B,"rm -rf '%s'",fp); (void)!system(c);
        printf("\xe2\x9c\x93 rm %s\n",argv[3]); return 0;
    }

    if(!strcmp(sub,"run")) {
        if(argc<5){fprintf(stderr,"Usage: a fork run <name> <cmd...>\n");return 1;}
        char fp[P]; snprintf(fp,P,"%s/%s",fd,argv[3]);
        struct stat st; if(stat(fp,&st)){fprintf(stderr,"x %s?\n",argv[3]);return 1;}
        if(chdir(fp)){perror(fp);return 1;}
        execvp(argv[4],argv+4);perror(argv[4]);return 1;
    }

    /* a fork <name> — create via local copy, keeps git remote for direct push */
    char fp[P]; snprintf(fp,P,"%s/%s",fd,sub);
    struct stat st; if(!stat(fp,&st)){printf("exists: %s\n",fp);
        char tf[P];snprintf(tf,P,"%s/cd_target",DDIR);writef(tf,fp);return 0;}
    char c[B]; snprintf(c,B,"cp -r '%s' '%s'",SDIR,fp);
    if(system(c)){fprintf(stderr,"x copy failed\n");return 1;}
    snprintf(c,B,"rm -f '%s/adata' && ln -s '%s' '%s/adata'",fp,AROOT,fp); (void)!system(c);
    printf("\xe2\x9c\x93 %s\n",fp);
    char tf[P];snprintf(tf,P,"%s/cd_target",DDIR);writef(tf,fp);
    return 0;
}
