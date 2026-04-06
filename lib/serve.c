/* a serve [port] — C HTTP server for UI. sub-1us response. */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#ifndef __APPLE__
#include <pty.h>
#else
#include <util.h>
#endif
/* minimal SHA-1 for WebSocket handshake — avoids openssl dependency */
static void _sha1(const unsigned char*d,size_t n,unsigned char out[20]){
    uint32_t h0=0x67452301,h1=0xEFCDAB89,h2=0x98BADCFE,h3=0x10325476,h4=0xC3D2E1F0;
    size_t ml=n*8,pl=((56-((n+1)%64))%64),tl=n+1+pl+8;
    unsigned char*m=calloc(tl,1);memcpy(m,d,n);m[n]=0x80;
    for(int i=0;i<8;i++)m[tl-1-i]=(unsigned char)(ml>>(i*8));
    for(size_t i=0;i<tl;i+=64){
        uint32_t w[80],a=h0,b=h1,c=h2,dd2=h3,e=h4;
        for(int j=0;j<16;j++)w[j]=(uint32_t)(m[i+j*4]<<24|m[i+j*4+1]<<16|m[i+j*4+2]<<8|m[i+j*4+3]);
        for(int j=16;j<80;j++){uint32_t t=w[j-3]^w[j-8]^w[j-14]^w[j-16];w[j]=(t<<1)|(t>>31);}
        for(int j=0;j<80;j++){uint32_t f,k;
            if(j<20){f=(b&c)|((~b)&dd2);k=0x5A827999;}
            else if(j<40){f=b^c^dd2;k=0x6ED9EBA1;}
            else if(j<60){f=(b&c)|(b&dd2)|(c&dd2);k=0x8F1BBCDC;}
            else{f=b^c^dd2;k=0xCA62C1D6;}
            uint32_t t=((a<<5)|(a>>27))+f+e+k+w[j];e=dd2;dd2=c;c=(b<<30)|(b>>2);b=a;a=t;}
        h0+=a;h1+=b;h2+=c;h3+=dd2;h4+=e;}
    free(m);
    for(int i=0;i<4;i++){out[i]=(unsigned char)(h0>>(24-i*8));out[4+i]=(unsigned char)(h1>>(24-i*8));
        out[8+i]=(unsigned char)(h2>>(24-i*8));out[12+i]=(unsigned char)(h3>>(24-i*8));out[16+i]=(unsigned char)(h4>>(24-i*8));}
}

static char *_html_cache;static int _html_len;
static void _html_gen(void){
    char c[B];snprintf(c,B,"python3 -c \"import sys;sys.path.insert(0,'%s/lib');"
        "from ui.ui_full import spa,web;import asyncio;"
        "r=asyncio.run(spa(None));print(r.text,end='')\" 2>/dev/null",SDIR);
    FILE*f=popen(c,"r");if(!f)return;
    int cap=65536,len=0;_html_cache=malloc((size_t)cap);
    while((_html_len=len+(int)fread(_html_cache+len,1,(size_t)(cap-len),f))>len){len=_html_len;if(len>=cap-1){cap*=2;_html_cache=realloc(_html_cache,(size_t)cap);}}
    pclose(f);_html_len=len;
}
static void _sresp(int c,int code,const char*ct,const char*body,int bl){
    char h[256];int hl=snprintf(h,256,"HTTP/1.1 %d OK\r\nContent-Type:%s\r\nContent-Length:%d\r\nConnection:keep-alive\r\nAccess-Control-Allow-Origin:*\r\n\r\n",code,ct,bl);
    (void)!write(c,h,(size_t)hl);(void)!write(c,body,(size_t)bl);
}
/* WebSocket: upgrade handshake */
static int _ws_upgrade(int c,const char*req){
    const char*k=strstr(req,"Sec-WebSocket-Key: ");if(!k)return 0;
    k+=19;char key[64];int i=0;while(k[i]&&k[i]!='\r'&&i<60){key[i]=k[i];i++;}
    snprintf(key+i,64-i,"258EAFA5-E914-47DA-95CA-5AB5DC595305");
    unsigned char sha[20];_sha1((unsigned char*)key,(size_t)(i+36),sha);
    /* base64 */
    static const char*b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char acc[32];int j=0;
    for(int p=0;p<18;p+=3){unsigned v=(unsigned)(sha[p]<<16|sha[p+1]<<8|sha[p+2]);
        acc[j++]=b64[v>>18&63];acc[j++]=b64[v>>12&63];acc[j++]=b64[v>>6&63];acc[j++]=b64[v&63];}
    {unsigned v=(unsigned)(sha[18]<<16|sha[19]<<8);acc[j++]=b64[v>>18&63];acc[j++]=b64[v>>12&63];acc[j++]=b64[v>>6&63];acc[j++]='=';}
    acc[j]=0;
    char r[256];int rl=snprintf(r,256,"HTTP/1.1 101 Switching Protocols\r\nUpgrade:websocket\r\nConnection:Upgrade\r\nSec-WebSocket-Accept:%s\r\n\r\n",acc);
    (void)!write(c,r,(size_t)rl);return 1;
}
static void _ws_send(int c,const char*d,int n){
    unsigned char h[10];int hl=2;h[0]=0x81;
    if(n<126){h[1]=(unsigned char)n;}
    else{h[1]=126;h[2]=(unsigned char)(n>>8);h[3]=(unsigned char)(n&0xFF);hl=4;}
    (void)!write(c,h,(size_t)hl);(void)!write(c,d,(size_t)n);
}
static int _ws_recv(int c,char*buf,int bsz){
    unsigned char h[2];if(read(c,h,2)!=2)return -1;
    int op=h[0]&0x0F,mask=h[1]&0x80,len=h[1]&0x7F;
    if(op==8)return -1; /* close */
    if(len==126){unsigned char e[2];(void)!read(c,e,2);len=(e[0]<<8)|e[1];}
    if(len>=bsz)len=bsz-1;
    unsigned char mk[4]={0,0,0,0};if(mask)(void)!read(c,mk,4);
    (void)!read(c,buf,(size_t)len);
    if(mask)for(int i=0;i<len;i++)buf[i]^=(char)mk[i%4];
    buf[len]=0;return len;
}
/* PTY terminal over WebSocket */
static void _ws_term(int c){
    int m,s;if(openpty(&m,&s,NULL,NULL,NULL)<0)return;
    pid_t p=fork();
    if(!p){close(m);setsid();ioctl(s,TIOCSCTTY,0);
        dup2(s,0);dup2(s,1);dup2(s,2);close(s);
        char*env[]={"TERM=xterm-256color","PATH=/usr/local/bin:/usr/bin:/bin",NULL};
        execle("/bin/bash","bash","-l",(char*)0,env);_exit(1);}
    close(s);
    struct pollfd pf[2]={{c,POLLIN,0},{m,POLLIN,0}};
    char buf[4096];
    while(poll(pf,2,-1)>0){
        if(pf[1].revents&POLLIN){int n=(int)read(m,buf,4096);if(n<=0)break;_ws_send(c,buf,n);}
        if(pf[0].revents&POLLIN){int n=_ws_recv(c,buf,4096);if(n<0)break;
            /* check for resize JSON */
            if(buf[0]=='{'){char*co=strstr(buf,"\"cols\":");char*ro=strstr(buf,"\"rows\":");
                if(co&&ro){struct winsize w={.ws_row=(unsigned short)atoi(ro+6),.ws_col=(unsigned short)atoi(co+6)};ioctl(m,TIOCSWINSZ,&w);continue;}}
            (void)!write(m,buf,(size_t)n);}
        if(pf[0].revents&(POLLHUP|POLLERR)||pf[1].revents&(POLLHUP|POLLERR))break;
    }
    kill(p,SIGHUP);close(m);waitpid(p,NULL,0);
}
static void _handle(int c){
    char req[8192];int n=(int)read(c,req,8191);if(n<=0)return;req[n]=0;
    int one=1;setsockopt(c,IPPROTO_TCP,TCP_NODELAY,&one,4);
    if(!strncmp(req,"GET / ",6)||!strncmp(req,"GET /jobs",9)||!strncmp(req,"GET /note",9)||!strncmp(req,"GET /tasks",10)||!strncmp(req,"GET /term",9)){
        if(_html_cache)_sresp(c,200,"text/html",_html_cache,_html_len);
        else _sresp(c,503,"text/plain","generating",10);return;}
    if(!strncmp(req,"GET /ws",7)&&strstr(req,"Upgrade: websocket")){
        if(_ws_upgrade(c,req))_ws_term(c);return;}
    if(!strncmp(req,"GET /api/u-status",17)){_sresp(c,200,"application/json","{\"ok\":true}",11);return;}
    if(!strncmp(req,"POST /api/omni",14)){
        char*body=strstr(req,"\r\n\r\n");if(!body){_sresp(c,400,"text/plain","bad",3);return;}
        body+=4;char*q=strstr(body,"q=");if(!q){_sresp(c,400,"text/plain","no q",4);return;}
        q+=2;char cmd[512];int i=0;
        for(;q[i]&&q[i]!='&'&&i<510;i++){if(q[i]=='+')cmd[i]=' ';else if(q[i]=='%'&&q[i+1]&&q[i+2]){
            char h[3]={q[i+1],q[i+2],0};cmd[i]=(char)strtol(h,NULL,16);i-=0;q+=2;}else cmd[i]=q[i];}cmd[i]=0;
        int pp[2];pipe(pp);pid_t ch=fork();
        if(!ch){close(pp[0]);dup2(pp[1],1);dup2(pp[1],2);close(pp[1]);
            char*a[]={"a",NULL,NULL};char w[512];snprintf(w,512,"%s",cmd);
            char*sp=strchr(w,' ');if(sp){*sp=0;a[1]=w;/* only first word */}else a[1]=w;
            execvp("a",a);_exit(1);}
        close(pp[1]);char out[8192];int ol=0;
        {int r;while((r=(int)read(pp[0],out+ol,(size_t)(8191-ol)))>0)ol+=r;}
        close(pp[0]);waitpid(ch,NULL,0);out[ol]=0;
        /* wrap in <pre> like aiohttp version */
        char resp[16384];int rl=snprintf(resp,16384,"<pre style=\"color:#fff\">%.*s</pre>",ol,out);
        _sresp(c,200,"text/html",resp,rl);return;}
    _sresp(c,404,"text/plain","not found",9);
}
static int cmd_serve(int argc,char**argv){perf_disarm();signal(SIGPIPE,SIG_IGN);signal(SIGCHLD,SIG_IGN);
    int port=argc>2?atoi(argv[2]):1111;
    /* kill existing python UI */
    (void)!system("pkill -f 'ui.ui_full\\|ui_full.py' 2>/dev/null");usleep(200000);
    printf("> generating HTML...\n");_html_gen();
    if(!_html_cache){puts("x HTML generation failed");return 1;}
    printf("+ %d bytes cached\n",_html_len);
    int fd=socket(AF_INET,SOCK_STREAM,0);
    int one=1;setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,4);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons((uint16_t)port),.sin_addr={htonl(0x7f000001)}};
    if(bind(fd,(void*)&a,sizeof a)<0){perror("bind");free(_html_cache);return 1;}
    listen(fd,64);printf("+ http://localhost:%d (C server, pid %d)\n",port,(int)getpid());
    for(;;){int c=accept(fd,0,0);if(c<0)continue;
        /* WebSocket needs its own process (blocking pty loop) */
        char peek[8];ssize_t pn=recv(c,peek,7,MSG_PEEK);
        if(pn>0&&!strncmp(peek,"GET /ws",7)){
            if(!fork()){close(fd);_handle(c);close(c);_exit(0);}close(c);
        }else{_handle(c);close(c);}}
}
