/* a vm [run|ssh|kill|test] [debian|arch|fedora] — disposable QEMU VM */
typedef struct{const char*name,*url,*user,*cloudinit;}vmos_t;
static const vmos_t VMOS[]={
    {"debian","https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-genericcloud-amd64.qcow2","debian",
     "#cloud-config\npassword: %s\nchpasswd: {expire: false}\nssh_pwauth: true\npackages: [git,curl,tmux,openssh-server]\nruncmd: [systemctl enable --now ssh]\n"},
    {"arch","https://geo.mirror.pkgbuild.com/images/latest/Arch-Linux-x86_64-cloudimg.qcow2","arch",
     "#cloud-config\npassword: %s\nchpasswd: {expire: false}\nssh_pwauth: true\npackages: [git,curl,tmux,openssh]\nruncmd: [systemctl enable --now sshd]\n"},
    {"fedora","https://download.fedoraproject.org/pub/fedora/linux/releases/42/Cloud/x86_64/images/Fedora-Cloud-Base-Generic-42-1.1.x86_64.qcow2","fedora",
     "#cloud-config\npassword: %s\nchpasswd: {expire: false}\nssh_pwauth: true\npackages: [git,curl,tmux,openssh-server]\nruncmd: [systemctl enable --now sshd]\n"},
};
#define NVMOS (sizeof(VMOS)/sizeof(*VMOS))
static const vmos_t*vm_find(const char*n){for(int i=0;i<(int)NVMOS;i++)if(!strcmp(VMOS[i].name,n))return&VMOS[i];return&VMOS[0];}

static int cmd_vm(int argc, char **argv) {
    const char*sub=argc>2?argv[2]:"run";
    /* find OS arg */
    const char*osn="debian";
    for(int i=2;i<argc;i++)for(int j=0;j<(int)NVMOS;j++)if(!strcmp(argv[i],VMOS[j].name)){osn=argv[i];break;}
    const vmos_t*os=vm_find(osn);
    char d[P],img[P],seed[P],log[P],ud[P],sd[P],md[P];
    snprintf(d,P,"%s/vm",AROOT);snprintf(img,P,"%s/%s.qcow2",d,os->name);
    snprintf(seed,P,"%s/%s-seed.iso",d,os->name);snprintf(log,P,"%s/console.log",d);
    const char*port="2222",*pw="testvm1";
    char usr[128];snprintf(usr,128,"%s@localhost",os->user);
    perf_disarm();
    if(!strcmp(sub,"kill")){char c[B];snprintf(c,B,"pkill -f hostfwd=tcp::%s",port);return system(c);}
    if(!strcmp(sub,"test")){
        char cmd[B];
        snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -p %s %s 'echo ok' 2>/dev/null",pw,port,usr);
        if(system(cmd)){char*rv[]={"a","vm","run",(char*)osn,NULL};if(cmd_vm(4,rv))return 1;}
        printf("> replicating a...\n");
        snprintf(cmd,B,"a new -p %s %s:%s",pw,usr,port);system(cmd);
        printf("> setting up claude auth...\n");
        char kf[P];snprintf(kf,P,"%s/git/login/api_keys.env",AROOT);
        char*keys=readf(kf,NULL);
        if(keys){char*ak=strstr(keys,"ANTHROPIC_API_KEY=");
            if(ak){char val[256];int i=0;ak+=18;while(ak[i]&&ak[i]!='\n'&&i<255){val[i]=ak[i];i++;}val[i]=0;
                snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'echo export ANTHROPIC_API_KEY=%s >> ~/.bashrc'",pw,port,usr,val);
                system(cmd);}free(keys);}
        printf("> ensuring claude installed...\n");
        snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'export PATH=$HOME/.local/bin:$PATH && (command -v claude >/dev/null || npm install -g @anthropic-ai/claude-code) 2>&1'",pw,port,usr);
        system(cmd);
        printf("> running claude on codebase...\n");
        snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'export PATH=$HOME/.local/bin:$HOME/.claude/local/bin:$PATH && export ANTHROPIC_API_KEY=$(grep ANTHROPIC ~/a/adata/git/login/api_keys.env|cut -d= -f2) && cd ~/a && claude -p --dangerously-skip-permissions \"Read the codebase with a cat 3. Describe what this project is and list all commands.\"' 2>&1",pw,port,usr);
        system(cmd);
        printf("\n> tearing down VM...\n");
        snprintf(cmd,B,"pkill -f hostfwd=tcp::%s",port);system(cmd);
        puts("+ done");return 0;
    }
    if(!strcmp(sub,"ssh")){
        char*a[]={"sshpass","-p",(char*)pw,"ssh","-o","StrictHostKeyChecking=no","-o","UserKnownHostsFile=/dev/null","-o","ConnectTimeout=10","-o","ServerAliveInterval=10","-p",(char*)port,usr,NULL};
        char*a2[20];int n=0;for(int i=0;a[i];i++)a2[n++]=a[i];
        for(int i=3;i<argc&&n<19;i++)if(strcmp(argv[i],osn))a2[n++]=argv[i];
        a2[n]=NULL;execvp(a2[0],a2);return 1;}
    mkdirp(d);
    char c[B];
    if(!fexists(img)){
        printf("> downloading %s cloud image...\n",os->name);
        if(system("command -v qemu-system-x86_64 >/dev/null")){puts("x install qemu: sudo pacman -S qemu-full cdrtools");return 1;}
        snprintf(c,B,"curl -fsSLo '%s' '%s'&&qemu-img resize '%s' 20G",img,os->url,img);
        if(system(c)){puts("x download failed");return 1;}
    }
    snprintf(ud,P,"%s/%s-user-data",d,os->name);
    {FILE*f=fopen(ud,"w");if(!f)return 1;fprintf(f,os->cloudinit,pw);fclose(f);}
    snprintf(sd,P,"%s/%s-seed",d,os->name);mkdirp(sd);
    snprintf(c,B,"cp '%s' '%s/user-data'",ud,sd);system(c);
    snprintf(md,P,"%s/meta-data",sd);writef(md,"{\"instance-id\":\"vm0\"}");
    snprintf(c,B,"xorriso -as mkisofs -o '%s' -V cidata -J -r '%s/' 2>&1",seed,sd);
    if(system(c)){puts("x xorriso failed");return 1;}
    pid_t p=fork();
    if(p==0){int fd=open(log,O_WRONLY|O_CREAT|O_TRUNC,0644);if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
        char di[P],ds[P];snprintf(di,P,"file=%s,format=qcow2",img);snprintf(ds,P,"file=%s,format=raw",seed);
        execlp("qemu-system-x86_64","qemu-system-x86_64","-m","4G","-smp","2","-drive",di,"-drive",ds,"-device","virtio-net-pci,netdev=n0","-netdev","user,id=n0,hostfwd=tcp::2222-:22","-nographic","-enable-kvm",(char*)0);_exit(1);}
    printf("booting %s (pid %d)...\n",os->name,(int)p);
    for(int i=0;i<12;i++){sleep(10);
        char c[B];snprintf(c,B,"sshpass -p %s ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -p %s %s 'echo ready' 2>/dev/null",pw,port,usr);
        if(!system(c))return 0;
        printf("  waiting (%ds)...\n",(i+1)*10);}
    printf("x timeout — check %s\n",log);return 1;
}
