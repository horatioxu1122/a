/* a new [-p pw] <user@host[:port]> — replicate a onto target device via ssh */
static int cmd_new(int argc, char **argv) {
    if(argc<3){puts("Usage: a new [-p pw] <user@host[:port]>");return 1;}
    perf_disarm();
    int ai=2;const char*pw=NULL;
    if(argc>3&&!strcmp(argv[2],"-p")){pw=argv[3];ai=4;}
    if(ai>=argc){puts("x need target");return 1;}
    char hp[256],port[8];snprintf(hp,256,"%s",argv[ai]);
    char*c=strrchr(hp,':');if(c&&strlen(c+1)<=5&&atoi(c+1)>0){snprintf(port,8,"%s",c+1);*c=0;}else snprintf(port,8,"22");
    char so[256];snprintf(so,256,"-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10 -p %s",port);
    if(pw)setenv("SSHPASS",pw,1);
    char sp[32]="";if(pw)snprintf(sp,32,"sshpass -e ");
    char cmd[B];snprintf(cmd,B,"%sssh %s %s 'echo ok' 2>/dev/null",sp,so,hp);
    if(system(cmd)){printf("x %s:%s\n",hp,port);return 1;}
    printf("+ %s:%s sync+install...\n",hp,port);
    snprintf(cmd,B,"tar -C '%s' --exclude='adata/vm' --exclude='adata/local' --exclude='.git' -czf - . | %sssh %s %s 'mkdir -p ~/a&&tar -C ~/a -xzf -&&sh ~/a/a.c install' 2>&1",SDIR,sp,so,hp);
    int r=system(cmd);
    char tok[512]="",nm[128]="",em[128]="";int l;
    pcmd("gh auth token 2>/dev/null",tok,512);tok[strcspn(tok,"\n")]=0;
    pcmd("git config --global user.name",nm,128);nm[strcspn(nm,"\n")]=0;
    pcmd("git config --global user.email",em,128);em[strcspn(em,"\n")]=0;
    l=snprintf(cmd,B,"%sssh %s %s 'export PATH=$HOME/.local/bin:$PATH;",sp,so,hp);
    if(tok[0])l+=snprintf(cmd+l,(size_t)(B-l)," echo %s|gh auth login --with-token 2>/dev/null&&gh auth setup-git 2>/dev/null;",tok);
    if(nm[0])l+=snprintf(cmd+l,(size_t)(B-l)," git config --global user.name \"%s\";",nm);
    if(em[0])l+=snprintf(cmd+l,(size_t)(B-l)," git config --global user.email \"%s\";",em);
    snprintf(cmd+l,(size_t)(B-l),"' 2>&1");system(cmd);
    puts(r?"! warnings":"+ done");
    return r?1:0;
}
