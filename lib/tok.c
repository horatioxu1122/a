/* tok — sum bytes/4 across files/dirs (matches a.c footer convention) */
static int cmd_tok(int c,char**v){perf_disarm();
    if(c<3){puts("Usage: a tok <file|dir>...");return 1;}
    long total=0;
    for(int i=2;i<c;i++){
        char cm[P*2],buf[64]="";
        snprintf(cm,sizeof(cm),"find '%s' -type f ! -path '*/.git/*' -exec wc -c {} + 2>/dev/null|awk 'END{print $1+0}'",v[i]);
        pcmd(cm,buf,64);long t=atol(buf)/4;total+=t;
        printf("%10ld  %s\n",t,v[i]);}
    if(c>3)printf("%10ld  total\n",total);
    return 0;}
