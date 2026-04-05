/* a vm [run|ssh|kill] — disposable QEMU VM for safe command testing */
static int cmd_vm(int argc, char **argv) {
    char d[P],img[P],seed[P],log[P],ud[P],sd[P],md[P];
    snprintf(d,P,"%s/vm",AROOT);snprintf(img,P,"%s/test.qcow2",d);
    snprintf(seed,P,"%s/seed.iso",d);snprintf(log,P,"%s/console.log",d);
    const char*port="2222",*pw="testvm1",*usr="debian@localhost";
    const char*so[]={"sshpass","-p",(char*)pw,"ssh","-o","StrictHostKeyChecking=no","-o","UserKnownHostsFile=/dev/null","-o","ConnectTimeout=10","-o","ServerAliveInterval=10","-p",(char*)port,(char*)usr,NULL};
    const char*sub=argc>2?argv[2]:"run";
    perf_disarm();
    if(!strcmp(sub,"kill")){char pat[64];snprintf(pat,64,"hostfwd=tcp::%s",port);
        execlp("pkill","pkill","-f",pat,(char*)0);return 1;}
    if(!strcmp(sub,"test")){
        /* full cycle: boot → replicate → claude reads codebase → teardown */
        char cmd[B];
        /* boot if not running */
        snprintf(cmd,B,"sshpass -p '%s' ssh %s -p %s %s 'echo ok' 2>/dev/null",pw,"-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5",port,usr);
        if(system(cmd)){char*rv[]={"a","vm","run",NULL};cmd_vm(3,rv);}
        /* replicate */
        printf("> replicating a...\n");
        snprintf(cmd,B,"a new -p %s %s:%s",pw,usr,port);
        system(cmd);
        /* setup claude auth */
        printf("> setting up claude auth...\n");
        char kf[P];snprintf(kf,P,"%s/git/login/api_keys.env",AROOT);
        char*keys=readf(kf,NULL);
        if(keys){char*ak=strstr(keys,"ANTHROPIC_API_KEY=");
            if(ak){char val[256];int i=0;ak+=18;while(ak[i]&&ak[i]!='\n'&&i<255){val[i]=ak[i];i++;}val[i]=0;
                snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'echo export ANTHROPIC_API_KEY=%s >> ~/.bashrc'",pw,port,usr,val);
                system(cmd);}free(keys);}
        /* ensure claude installed */
        printf("> ensuring claude installed...\n");
        snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'export PATH=$HOME/.local/bin:$PATH && (command -v claude >/dev/null || npm install -g @anthropic-ai/claude-code) 2>&1'",pw,port,usr);
        system(cmd);
        /* run claude on codebase */
        printf("> running claude on codebase...\n");
        snprintf(cmd,B,"sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p %s %s 'export PATH=$HOME/.local/bin:$HOME/.claude/local/bin:$PATH && export ANTHROPIC_API_KEY=$(grep ANTHROPIC ~/a/adata/git/login/api_keys.env|cut -d= -f2) && cd ~/a && claude -p --dangerously-skip-permissions \"Read the codebase with a cat 3. Describe what this project is and list all commands.\"' 2>&1",pw,port,usr);
        system(cmd);
        /* teardown */
        printf("\n> tearing down VM...\n");
        char pat[64];snprintf(pat,64,"hostfwd=tcp::%s",port);
        char*kv[]={"pkill","-f",pat,NULL};
        pid_t p2=fork();if(!p2){execvp("pkill",kv);_exit(1);}
        int st;waitpid(p2,&st,0);
        puts("+ done");return 0;
    }
    if(!strcmp(sub,"ssh")){char*a[20];int n=0;
        for(int i=0;so[i];i++)a[n++]=(char*)so[i];
        for(int i=3;i<argc&&n<19;i++)a[n++]=argv[i];a[n]=NULL;
        execvp(a[0],a);return 1;}
    /* run */
    mkdirp(d);
    if(!fexists(img)){
        printf("> downloading debian cloud image...\n");
        if(system("command -v qemu-system-x86_64 >/dev/null")){puts("x install qemu first: sudo pacman -S qemu-full cdrtools");return 1;}
        char c[B];snprintf(c,B,"curl -fsSLo '%s' https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-genericcloud-amd64.qcow2&&qemu-img resize '%s' 10G",img,img);
        if(system(c)){puts("x download failed");return 1;}
        snprintf(ud,P,"%s/user-data",d);
        {FILE*f=fopen(ud,"w");if(!f)return 1;
        fprintf(f,"#cloud-config\npassword: %s\nchpasswd: {expire: false}\nssh_pwauth: true\npackages: [git,curl,tmux,openssh-server]\nruncmd: [systemctl enable --now ssh]\n",pw);fclose(f);}
        snprintf(sd,P,"%s/seed",d);mkdirp(sd);
        snprintf(c,B,"cp '%s' '%s/user-data'",ud,sd);system(c);
        snprintf(md,P,"%s/meta-data",sd);writef(md,"{\"instance-id\":\"vm0\"}");
        snprintf(c,B,"mkisofs -o '%s' -V cidata -J -r '%s/' 2>&1",seed,sd);
        if(system(c)){puts("x mkisofs failed");return 1;}
    }
    /* boot background */
    pid_t p=fork();
    if(p==0){int fd=open(log,O_WRONLY|O_CREAT|O_TRUNC,0644);if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
        char di[P],ds[P];snprintf(di,P,"file=%s,format=qcow2",img);snprintf(ds,P,"file=%s,format=raw",seed);
        execlp("qemu-system-x86_64","qemu-system-x86_64","-m","4G","-smp","2","-drive",di,"-drive",ds,"-device","virtio-net-pci,netdev=n0","-netdev","user,id=n0,hostfwd=tcp::2222-:22","-nographic","-enable-kvm",(char*)0);_exit(1);}
    printf("booting (pid %d)...\n",(int)p);
    for(int i=0;i<12;i++){sleep(10);
        char c[B];snprintf(c,B,"sshpass -p %s ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -p %s %s 'echo ready' 2>/dev/null",pw,port,usr);
        if(!system(c))return 0;
        printf("  waiting (%ds)...\n",(i+1)*10);}
    printf("x timeout — check %s\n",log);return 1;
}
