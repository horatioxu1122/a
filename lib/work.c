static int cmd_w(int c,char**v){AB;init_paths();
    char f[P],*hl[32];int n=0,dl=snprintf(f,P,"%s/workcycle",SROOT);mkdirp(f);
    strcpy(f+dl,"/habits.txt");
    if(!fexists(f))writef(f,"garlic\nexercise\nnoot\neggs\ncold_shower\n");
    char*h=readf(f,NULL);if(!h)return 1;
    for(char*l=h;*l&&n<32;l++){char*p=strchr(l,'\n');if(p)*p=0;if(*l)hl[n++]=l;l=p?p:l+strlen(l);}
    {char s[12];strftime(s,12,"%F",localtime(&(time_t){time(0)}));snprintf(f+dl,P-dl,"/%s.txt",s);}
    char*dn=readf(f,NULL);
    if(c<3){for(int i=0;i<n;i++)printf(" %d.%s %s\n",i+1,dn&&strstr(dn,hl[i])?"\xe2\x9c\x93":"[ ]",hl[i]);
        puts(" a w <#|name>");}
    else{int i=atoi(v[2]);char*s=i>0&&i<=n?hl[i-1]:v[2];
        {FILE*o=fopen(f,"a");if(o){fputs(s,o);fputc('\n',o);fclose(o);}}
        sync_bg();printf("\xe2\x9c\x93 %s\n",s);}free(h);free(dn);return 0;}
