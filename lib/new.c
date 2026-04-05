/* a new [-p pw] <user@host[:port]> — replicate a onto target device via ssh */
static int cmd_new(int argc, char **argv) {
    if(argc<3){puts("Usage: a new [-p password] <user@host[:port]>\n  a new -p testvm1 debian@localhost:2222");return 1;}
    perf_disarm();
    int ai=2;const char*pw=NULL;
    if(argc>3&&!strcmp(argv[2],"-p")){pw=argv[3];ai=4;}
    if(ai>=argc){puts("x need target");return 1;}
    char hp[256],port[8];snprintf(hp,256,"%s",argv[ai]);
    char*c=strrchr(hp,':');if(c&&strlen(c+1)<=5&&atoi(c+1)>0){snprintf(port,8,"%s",c+1);*c=0;}else snprintf(port,8,"22");
    char so[256];snprintf(so,256,"-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10 -p %s",port);
    if(pw)setenv("SSHPASS",pw,1);
    char sp[32]="";if(pw)snprintf(sp,32,"sshpass -e ");
    /* test connection */
    char cmd[B];snprintf(cmd,B,"%sssh %s %s 'echo ok' 2>/dev/null",sp,so,hp);
    if(system(cmd)){printf("x can't reach %s:%s\n",hp,port);return 1;}
    printf("+ connected to %s:%s\n",hp,port);
    /* tar+ssh a/ to target (no rsync needed) */
    printf("> syncing ~/a...\n");
    snprintf(cmd,B,"tar -C '%s' --exclude='adata/vm' --exclude='adata/local' --exclude='.git' -czf - . | %sssh %s %s 'mkdir -p ~/a && tar -C ~/a -xzf -'",SDIR,sp,so,hp);
    {int r2=system(cmd);if(r2&&WEXITSTATUS(r2)>1){puts("x sync failed");return 1;}}
    puts("+ synced");
    /* install on target */
    printf("> installing...\n");
    snprintf(cmd,B,"%sssh %s %s 'sh ~/a/a.c install' 2>&1",sp,so,hp);
    int r=system(cmd);
    if(r)puts("! install had warnings");else puts("+ installed");
    /* capture device info */
    char dev[P];snprintf(dev,P,"%s/git/devices",AROOT);mkdirp(dev);
    snprintf(cmd,B,"%sssh %s %s 'hostname;uname -m;cat /etc/os-release 2>/dev/null|head -3;which tmux git clang node claude 2>/dev/null' 2>/dev/null",sp,so,hp);
    char out[B];pcmd(cmd,out,B);
    char*nl=strchr(out,'\n');char hn[64]="unknown";if(nl){*nl=0;snprintf(hn,64,"%s",out);*nl='\n';}
    char df[P];snprintf(df,P,"%s/%s.txt",dev,hn);writef(df,out);
    printf("+ device: %s\n+ done — ssh to target and run: a\n",hn);
    return r?1:0;
}
