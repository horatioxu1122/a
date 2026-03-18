#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#define H(c) ((c)>='a'?(c)-87:(c)>='A'?(c)-55:(c)-48)
static void D(char*s){char*o=s;for(;*s;s++,o++)if(*s=='+'){*o=' ';}else if(*s=='%'&&s[1]&&s[2]){*o=H(s[1])*16+H(s[2]);s+=2;}else*o=*s;*o=0;}
int main(){int s=socket(AF_INET,SOCK_STREAM,0),c,o=1,f=open("lab/web.html",0);char b[65536],pg[65536],r[65536];int n=read(f,pg,sizeof pg);close(f);
setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&o,sizeof o);struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(1112)};bind(s,(void*)&a,sizeof a);listen(s,8);
while((c=accept(s,0,0))>=0){int z=read(c,b,sizeof b-1);b[z]=0;char*q;if(b[0]=='P'&&(q=strstr(b,"q="))){q+=2;char*e=strpbrk(q,"& \r\n");if(e)*e=0;D(q);char cmd[512];snprintf(cmd,512,"./adata/local/a %s 2>/dev/null|head -c 65000",q);FILE*p=popen(cmd,"r");int ol=fread(r,1,sizeof r,p);pclose(p);z=snprintf(b,512,"HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-Length:%d\r\n\r\n",ol+30);write(c,b,z);write(c,"<pre style=\"color:#fff\">",24);write(c,r,ol);write(c,"</pre>",6);}else{z=snprintf(b,256,"HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-Length:%d\r\n\r\n",n);write(c,b,z);write(c,pg,n);}close(c);}}
