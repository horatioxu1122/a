#if 0
cc -w -o "${0%.c}" "$0"&&exit 0
exit 1
#endif
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#define B 65536
static char*H[40];static int nh;
static void ha(const char*r,const char*c){if(nh>=40){free(H[0]);free(H[1]);memmove(H,H+2,38*sizeof(char*));nh-=2;}H[nh++]=strdup(r);H[nh++]=strdup(c);}
static void je(const char*s,char*o){char*p=o;for(;*s;s++)if(*s=='"'||*s=='\\'){*p++='\\';*p++=*s;}else if(*s=='\n'){*p++='\\';*p++='n';}else if(*(unsigned char*)s>31)*p++=*s;*p=0;}
static char ju(char**p,char*e){char*s=*p;if(*s!='\\'||s+1>=e){*p=s+1;return*s;}
if(s[1]=='u'&&s+5<e){int v=0;for(int i=2;i<6;i++){char h=s[i];v=v*16+(h<='9'?h-'0':(h|32)-'a'+10);}*p=s+6;return v<128?(char)v:'?';}
*p=s+2;return s[1]=='n'?'\n':s[1]=='t'?'\t':s[1];}
static char*chat(const char*sys){
static char b[B],r[B],e[B],ln[4096];int n,rl=0,th=0;
const char*mdl=getenv("G4M");if(!mdl)mdl="gemma4:e2b";
n=snprintf(b,B,"{\"model\":\"%s\",\"stream\":true,\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},",mdl,sys);
for(int i=0;i<nh;i+=2){je(H[i+1],e);n+=snprintf(b+n,B-n,"{\"role\":\"%s\",\"content\":\"%s\"},",H[i],e);}
if(b[n-1]==',')n--;snprintf(b+n,B-n,"]}");
FILE*f=fopen("/tmp/.g4b","w");if(!f)return 0;fputs(b,f);fclose(f);
f=popen("curl -s http://localhost:11434/api/chat -d@/tmp/.g4b","r");if(!f)return 0;
while(fgets(ln,4096,f)){char*p;int k;
if((p=strstr(ln,"\"thinking\":\""))){p+=12;k=1;}
else if((p=strstr(ln,"\"content\":\""))){p+=11;k=0;}
else continue;
char*q=p;while(*q&&!(*q=='"'&&*(q-1)!='\\'))q++;if(p==q)continue;*q=0;
if(k&&!th++)printf("\033[2m");
if(!k&&th){printf("\033[0m\n");th=0;}
for(char*s=p;*s;){char c=ju(&s,q);if(!k&&rl<B-1)r[rl++]=c;putchar(c);}
fflush(stdout);}
if(th)printf("\033[0m\n");
pclose(f);r[rl]=0;return r;}
static void ensure_ollama(void){
if(!system("curl -sf http://localhost:11434/api/tags>/dev/null 2>&1"))return;
printf("Starting ollama...\n");system("ollama serve >/dev/null 2>&1 &");
for(int i=0;i<30;i++){usleep(500000);if(!system("curl -sf http://localhost:11434/api/tags>/dev/null 2>&1"))return;}
fprintf(stderr,"x ollama failed to start\n");}
int main(int c,char**v){
int raw=0,ai=1,ntm=0;
for(int i=1;i<c;i++)if(!strcmp(v[i],"--raw")){raw=1;ai=i+1;}else if(!strcmp(v[i],"--notmux")){ntm=1;ai=i+1;}
if(!ntm&&!getenv("G4T")){char d[1024],cm[2048];getcwd(d,1024);
snprintf(cm,2048,"tmux %s 'G4T=1 %s/g4agent%s;exec $SHELL'",getenv("TMUX")?"new-window":"new-session -s g4",d,raw?" --raw":"");
execl("/bin/sh","sh","-c",cm,(char*)0);}
ensure_ollama();
const char*S=raw?"CMD:<cmd> or text.":"You run shell commands by replying CMD:<command>. Example: user says list files, you reply CMD:ls. After seeing output you can CMD: again or give a text answer. Always use the CMD: prefix to run commands.";
char in[B],out[B];
if(ai<c){int n=0;for(int i=ai;i<c;i++)n+=snprintf(in+n,B-n,"%s%s",i>ai?" ":"",v[i]);goto run;}
for(;;){printf("> ");fflush(stdout);
if(!fgets(in,B,stdin))break;in[strcspn(in,"\n")]=0;if(!*in)continue;
run:ha("user",in);
for(int i=0;i<20;i++){
char*t=chat(S);if(!t){puts("(err)");break;}
char*cp=t;while(*cp==' '||*cp=='\n')cp++;
if(strncmp(cp,"CMD:",4)){putchar('\n');ha("assistant",t);break;}cp+=4;
putchar('\n');char*cm=cp;while(*cm==' '||*cm=='`')cm++;
{char*e=cm;while(*e&&*e!='\n'&&*e!='`')e++;*e=0;}
printf("$ %s\n",cm);ha("assistant",t);
FILE*p=popen(cm,"r");int l=p?(int)fread(out,1,B-1,p):0;if(p)pclose(p);out[l]=0;
if(!*out)strcpy(out,"(no output)");printf("%s\n",out);
snprintf(in,B,"`%s`:\n%s",cm,out);ha("user",in);}
if(ai<c)break;}}
