#if 0
set -e
Z=$(command -v zig 2>/dev/null||echo /tmp/zig-linux-x86_64-0.13.0/zig)
[ -x "$Z" ]&&"$Z" cc -target x86_64-windows-gnu -w -o "${0%.c}.exe" "$0"&&echo "built ${0%.c}.exe"
cc -w -o "${0%.c}" "$0"&&echo "built ${0%.c}"
exit 0
#endif
/* vw.c — v7-style Unix shell, native Windows PE64 + POSIX.
 * Host fs IS the v7 fs (v7p "passthrough" model). Unix cmd names aliased on Windows. */
extern int system(const char*);
extern char*getenv(const char*);
extern int chdir(const char*);
#ifdef _WIN32
#define HV "USERPROFILE"
extern long _write(int,const void*,unsigned);
extern long _read(int,void*,unsigned);
extern void ExitProcess(unsigned);
#define W(b,n) _write(1,(b),(unsigned)(n))
#define R(b,n) _read(0,(b),(unsigned)(n))
#define EXIT(c) ExitProcess((unsigned)(c))
static const char*TX[][2]={{"ls","dir /b"},{"cat","type"},{"rm","del"},{"cp","copy"},{"mv","move"},{"pwd","cd"},{"clear","cls"},{"grep","findstr"},{"which","where"},{"ps","tasklist"},{0,0}};
#else
#define HV "HOME"
extern long write(int,const void*,unsigned long);
extern long read(int,void*,unsigned long);
extern void _exit(int) __attribute__((noreturn));
#define W(b,n) write(1,(b),(unsigned long)(n))
#define R(b,n) read(0,(b),(unsigned long)(n))
#define EXIT(c) _exit(c)
#endif
static int sl(const char*s){int n=0;while(s[n])n++;return n;}
static int sc(const char*a,const char*b){while(*a&&*a==*b)a++,b++;return *a-*b;}
static void p(const char*s){W(s,sl(s));}

int main(void){
    char*h=getenv(HV);if(h)chdir(h);
    p("vw — v7-style shell (native Windows/POSIX). 'help', 'exit'.\n");
    char ln[512];
    for(;;){
        p("# ");
        int i=0;char c;
        while(i<511){long r=R(&c,1);if(r<=0){p("\n");EXIT(0);}if(c=='\n')break;if(c!='\r')ln[i++]=c;}
        ln[i]=0;
        int k=0;while(ln[k]==' ')k++;if(!ln[k])continue;
        if(!sc(ln+k,"exit"))EXIT(0);
        if(!sc(ln+k,"help")){p("any host cmd runs. win aliases: ls cat rm cp mv pwd clear grep which ps\n");continue;}
#ifdef _WIN32
        int j=k;while(ln[j]&&ln[j]!=' ')j++;char s=ln[j];ln[j]=0;
        const char*r=0;for(int x=0;TX[x][0];x++)if(!sc(ln+k,TX[x][0])){r=TX[x][1];break;}
        ln[j]=s;
        if(r){char nl[768];int a=0,y=0;while(r[y])nl[a++]=r[y++];y=j;while(ln[y])nl[a++]=ln[y++];nl[a]=0;system(nl);continue;}
#endif
        system(ln+k);
    }
}
