/* a bench — measure UI server latency: every approach, table output */
#include <netinet/in.h>
#include <netinet/tcp.h>

static long _bns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000000000L+t.tv_nsec;}
typedef struct{long min,p50,p95,max;}blat_t;

static int _brecv(int s,char*buf,int bsz){
    int tot=0;
    for(int i=0;i<100;i++){
        int r=(int)read(s,buf+tot,(size_t)(bsz-1-tot));if(r<=0)break;tot+=r;buf[tot]=0;
        char*cl=strstr(buf,"Content-Length:");
        if(cl){int clen=atoi(cl+15);char*bd=strstr(buf,"\r\n\r\n");
            if(bd&&(tot-(int)(bd+4-buf))>=clen)return tot;}
        if(!cl&&strstr(buf,"\r\n\r\n"))return tot;
    }return tot;
}
static blat_t _bmeasure(int port,const char*req,int rlen,int N){
    long t[1024];if(N>1024)N=1024;char buf[8192];
    int s=socket(AF_INET,SOCK_STREAM,0);
    int one=1;setsockopt(s,IPPROTO_TCP,TCP_NODELAY,&one,4);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons((uint16_t)port),.sin_addr={htonl(0x7f000001)}};
    if(connect(s,(void*)&a,sizeof a)<0){close(s);return(blat_t){-1,-1,-1,-1};}
    for(int i=0;i<20;i++){(void)!write(s,req,(size_t)rlen);_brecv(s,buf,8192);}
    for(int i=0;i<N;i++){
        long t0=_bns();
        (void)!write(s,req,(size_t)rlen);_brecv(s,buf,8192);
        t[i]=(_bns()-t0)/1000; /* microseconds */
    }
    close(s);
    for(int i=0;i<N;i++)for(int j=i+1;j<N;j++)if(t[j]<t[i]){long x=t[i];t[i]=t[j];t[j]=x;}
    return(blat_t){t[0],t[N/2],t[N*95/100],t[N-1]};
}
static pid_t _bserv(int port,int mode){
    pid_t p=fork();if(p)return p;signal(SIGPIPE,SIG_IGN);
    int fd=socket(AF_INET,SOCK_STREAM,0);
    int one=1;setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,4);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons((uint16_t)port),.sin_addr={htonl(0x7f000001)}};
    bind(fd,(void*)&a,sizeof a);listen(fd,8);
    for(;;){
        int c=accept(fd,0,0);if(c<0)break;
        setsockopt(c,IPPROTO_TCP,TCP_NODELAY,&one,4);
        char buf[1024];
        while(read(c,buf,1024)>0){
            char body[4096]="ok";int bl=2;
            if(mode==1){int pp[2];pipe(pp);pid_t ch=fork();
                if(!ch){dup2(pp[1],1);close(pp[0]);close(pp[1]);execlp("a","a","help",(char*)0);_exit(1);}
                close(pp[1]);bl=(int)read(pp[0],body,4095);if(bl<0)bl=0;body[bl]=0;close(pp[0]);waitpid(ch,NULL,0);
            }else if(mode==2){DIR*d=opendir(".");struct dirent*e;bl=0;
                if(d){while((e=readdir(d))&&bl<4000)bl+=snprintf(body+bl,(size_t)(4095-bl),"%s\n",e->d_name);closedir(d);}}
            char hdr[128];int hl=snprintf(hdr,128,"HTTP/1.1 200 OK\r\nContent-Length:%d\r\nConnection:keep-alive\r\n\r\n",bl);
            (void)!write(c,hdr,(size_t)hl);(void)!write(c,body,(size_t)bl);
        }close(c);
    }_exit(0);
}
#define _BPR(nm,r) if(r.min>=0){if(r.p50<1)printf("%28s    <1us    <1us    <1us    <1us\n",nm); \
    else printf("%28s %7ldus %7ldus %7ldus %7ldus\n",nm,r.min,r.p50,r.p95,r.max);} \
    else printf("%28s %8s\n",nm,"N/A")
static int cmd_bench(int argc,char**argv){(void)argc;(void)argv;perf_disarm();signal(SIGPIPE,SIG_IGN);
    int N=200,cp=11120;
    const char*get="GET /api/u-status HTTP/1.1\r\nHost:l\r\nConnection:keep-alive\r\n\r\n";
    const char*post="POST /api/omni HTTP/1.1\r\nHost:l\r\nContent-Length:6\r\nContent-Type:application/x-www-form-urlencoded\r\nConnection:keep-alive\r\n\r\nq=help";
    const char*get2="GET / HTTP/1.1\r\nHost:l\r\nConnection:keep-alive\r\n\r\n";
    int gl=(int)strlen(get),pl=(int)strlen(post),g2l=(int)strlen(get2);
    printf("%28s %8s %8s %8s %8s\n","APPROACH","MIN","P50","P95","MAX");
    puts("──────────────────────────── ──────── ──────── ──────── ────────");
    {blat_t r=_bmeasure(1111,get,gl,N);_BPR("aiohttp echo",r);}
    {blat_t r=_bmeasure(1111,post,pl,N);_BPR("aiohttp + subprocess",r);}
    {pid_t s=_bserv(cp,0);usleep(50000);blat_t r=_bmeasure(cp,get2,g2l,N);_BPR("C echo",r);kill(s,SIGTERM);waitpid(s,NULL,0);}
    {pid_t s=_bserv(cp,1);usleep(50000);blat_t r=_bmeasure(cp,get2,g2l,N);_BPR("C + fork/exec",r);kill(s,SIGTERM);waitpid(s,NULL,0);}
    {pid_t s=_bserv(cp,2);usleep(50000);blat_t r=_bmeasure(cp,get2,g2l,N);_BPR("C + in-process readdir",r);kill(s,SIGTERM);waitpid(s,NULL,0);}
    /* unix socket — no HTTP, raw IPC */
    {int sv[2];socketpair(AF_UNIX,SOCK_STREAM,0,sv);pid_t p=fork();
    if(!p){close(sv[0]);char b[256];while(read(sv[1],b,256)>0)(void)!write(sv[1],"ok",2);close(sv[1]);_exit(0);}
    close(sv[1]);long t[1024];
    for(int i=0;i<20;i++){(void)!write(sv[0],"x",1);char b[8];(void)!read(sv[0],b,8);}
    for(int i=0;i<N;i++){long t0=_bns();(void)!write(sv[0],"x",1);char b[8];(void)!read(sv[0],b,8);t[i]=(_bns()-t0)/1000;}
    close(sv[0]);kill(p,SIGTERM);waitpid(p,NULL,0);
    for(int i=0;i<N;i++)for(int j=i+1;j<N;j++)if(t[j]<t[i]){long x=t[i];t[i]=t[j];t[j]=x;}
    blat_t r={t[0],t[N/2],t[N*95/100],t[N-1]};_BPR("unix socket echo",r);}
    /* direct call — no IPC at all */
    {long t[1024];char body[4096];
    for(int i=0;i<N;i++){long t0=_bns();DIR*d=opendir(".");struct dirent*e;int bl=0;
        if(d){while((e=readdir(d))&&bl<4000)bl+=snprintf(body+bl,(size_t)(4095-bl),"%s\n",e->d_name);closedir(d);}
        t[i]=(_bns()-t0)/1000;}
    for(int i=0;i<N;i++)for(int j=i+1;j<N;j++)if(t[j]<t[i]){long x=t[i];t[i]=t[j];t[j]=x;}
    blat_t r={t[0],t[N/2],t[N*95/100],t[N-1]};_BPR("direct readdir (no IPC)",r);}
    puts("\n+ browser paint: ~16000us @60fps, ~6000us @165hz");
    puts("+ sub-1ms server: in-process, no fork");
    puts("+ sub-1ms pixel: native render, no browser");
    return 0;
}
#undef _BPR
