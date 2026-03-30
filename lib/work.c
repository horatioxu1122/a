/* ── workcycle habits ── */
static int cmd_w(int c,char**v){
    AB;init_paths();char dir[P],df[P],hf[P];
    snprintf(dir,P,"%s/workcycle",SROOT);mkdirp(dir);
    snprintf(hf,P,"%s/habits.txt",dir);
    if(!fexists(hf)){FILE*f=fopen(hf,"w");if(f){fputs("garlic\nexercise\nnoot\neggs\ncold_shower\n",f);fclose(f);}sync_bg();}
    time_t now=time(NULL);struct tm*t=localtime(&now);char ds[16];
    strftime(ds,16,"%Y-%m-%d",t);snprintf(df,P,"%s/%s.txt",dir,ds);
    /* load habits into array for num lookup */
    char*hab=readf(hf,NULL),*done=readf(df,NULL);if(!hab){puts("no habits.txt");return 1;}
    char*hl[64];int nh=0;
    for(char*l=hab,*nl;l&&*l&&nh<64;l=nl){nl=strchr(l,'\n');if(nl)*nl++=0;if(*l)hl[nh++]=l;}
    /* mark by name or number */
    if(c>2){char*h=v[2];int idx=atoi(h);
        if(idx>=1&&idx<=nh)h=hl[idx-1];
        FILE*f=fopen(df,"a");if(f){fprintf(f,"%s\n",h);fclose(f);}
        sync_bg();printf("\xe2\x9c\x93 %s\n",h);free(hab);free(done);return 0;}
    /* show today */
    for(int i=0;i<nh;i++){int ok=done&&strstr(done,hl[i]);
        printf("  %d. %s %s\n",i+1,ok?"\xe2\x9c\x93":"[ ]",hl[i]);}
    printf("  a w <#|name>  check off\n");
    free(hab);free(done);return 0;
}
