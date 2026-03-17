/* Focus box TUI experiment — centered input box with suggestions on type.
   Empty screen = focus/meditation mode. First keystroke = command mode with
   suggestions below box and session count above. Backspace to empty = back to focus.
   Build: cc -O2 -o focus_box focus_box.c && ./focus_box
   Or integrate into cmd_i render path in lib/sess.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>

#define B 8192

/* Demo data */
static char *ITEMS[] = {
    "claude\tclaude","gemini\tgemini","codex\tcodex","cleanup\tcmd",
    "config\tcmd","copy\tcmd","create\tcmd","dash\tcmd","deps\tcmd",
    "diff\tcmd","done\tcmd","help\tcmd","install\tcmd","job\tcmd",
    "kill\tcmd","ls\tcmd","note\tcmd","push\tcmd","pull\tcmd","ssh\tcmd",
    NULL
};

static int strcasehas(const char *h, const char *n) {
    for(;*h;h++){const char*a=h,*b=n;while(*a&&*b&&(*a|32)==(*b|32)){a++;b++;}if(!*b)return 1;}return 0;
}

int main(void) {
    struct winsize ws; ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    if(!ws.ws_col){ws.ws_col=80;ws.ws_row=24;}
    int bw=ws.ws_col>50?40:ws.ws_col-4,bx=(ws.ws_col-bw)/2+1,by=ws.ws_row/2;
    int maxshow=ws.ws_row-by-3; if(maxshow<3)maxshow=3;
    char hline[256];int hl=0;for(int i=0;i<bw-2;i++){memcpy(hline+hl,"\xe2\x94\x80",3);hl+=3;}hline[hl]=0;

    /* Count items */
    int n=0; while(ITEMS[n])n++;

    /* Raw mode */
    struct termios old,raw;
    tcgetattr(STDIN_FILENO,&old); raw=old;
    raw.c_lflag&=~(tcflag_t)(ICANON|ECHO|ISIG);
    raw.c_cc[VMIN]=1;raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO,TCSANOW,&raw);
    write(STDOUT_FILENO,"\033[?1000h\033[?1006h\033[2J",20);

    char buf[256]=""; int blen=0,sel=0;
    int nsess=3; /* demo value */

    while(1){
        /* Filter */
        char *matches[512]; int nm=0;
        for(int i=0;i<n&&nm<512;i++){
            if(blen){if(!strcasehas(ITEMS[i],buf))continue;}
            matches[nm++]=ITEMS[i];
        }
        if(sel>=nm)sel=nm?nm-1:0;
        int show=nm<maxshow?nm:maxshow;
        int top=sel>=maxshow?sel-maxshow+1:0;
        if(show>nm-top)show=nm-top;

        /* Render */
        {char fb[B*4];int fl=0;
        fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[?25l");
        if(blen)fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%d;%dH\033[90m%d sess\033[K\033[0m",by-1,bx+1,nsess);
        else fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%d;1H\033[K",by-1);
        fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%d;%dH\033[90m\xe2\x95\xad%s\xe2\x95\xae\033[%d;%dH\xe2\x94\x82\033[0m %s\033[K\033[%d;%dH\033[90m\xe2\x94\x82\033[%d;%dH\xe2\x95\xb0%s\xe2\x95\xaf\033[0m",by,bx,hline,by+1,bx,buf,by+1,bx+bw-1,by+2,bx,hline);
        if(blen)for(int i=0;i<show;i++){int j=top+i,W=ws.ws_col-bx;char*t=strchr(matches[j],'\t');int ml=t?(int)(t-matches[j]):(int)strlen(matches[j]);
            if(ml>W-5)ml=W-5;fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%d;%dH\033[K%s a %.*s",by+3+i,bx,j==sel?"\033[1m>":" ",ml,matches[j]);
            if(t&&bx+ml+8+(int)strlen(t+1)<(int)ws.ws_col)fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%dG\033[90m%s",ws.ws_col-(int)strlen(t+1),t+1);
            fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[0m");}
        fl+=snprintf(fb+fl,(size_t)(B*4-fl),"\033[%d;1H\033[J\033[%d;%dH\033[?25h",by+3+show,by+1,bx+2+blen);
        (void)!write(STDOUT_FILENO,fb,(size_t)fl);}

        /* Input */
        char ch; if(read(0,&ch,1)!=1)break;
        if(ch=='\x1b'){int av;usleep(50000);ioctl(0,FIONREAD,&av);if(!av)break;
            char seq[2];if(read(0,seq,1)!=1)break;
            if(seq[0]=='['){if(read(0,seq+1,1)!=1)break;
                if(seq[1]=='A'){if(sel>0)sel--;}
                else if(seq[1]=='B'){if(sel<nm-1)sel++;}
                else if(seq[1]=='<'){int mb=0,mx=0,my=0;char mc;
                    while(read(0,&mc,1)==1&&mc!=';')mb=mb*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!=';')mx=mx*10+mc-'0';
                    while(read(0,&mc,1)==1&&mc!='M'&&mc!='m')my=my*10+mc-'0';
                    (void)mx;if(mc=='M'&&mb==0){int ci=my-by-3+top;if(ci>=0&&ci<nm){sel=ci;}}
                    else if(mc=='M'&&(mb==64||mb==65)){if(mb==64&&sel>0)sel--;if(mb==65&&sel<nm-1)sel++;}}
            } else break;
        } else if(ch=='\t'){if(sel<nm-1)sel++;}
        else if(ch=='\x7f'||ch=='\b'){if(blen)buf[--blen]=0;sel=0;}
        else if(ch=='\r'||ch=='\n'){if(nm){
            char*m=matches[sel];char*t=strchr(m,'\t');int ml=t?(int)(t-m):(int)strlen(m);
            tcsetattr(STDIN_FILENO,TCSANOW,&old);
            write(STDOUT_FILENO,"\033[?1000l\033[?1006l",16);
            printf("\033[2J\033[HSelected: a %.*s\n",ml,m);
            return 0;}}
        else if(ch=='\x03'||ch=='\x04')break;
        else if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch==' '){if(blen<254){buf[blen++]=ch;buf[blen]=0;sel=0;}}
    }
    write(STDOUT_FILENO,"\033[?1000l\033[?1006l",16);
    tcsetattr(STDIN_FILENO,TCSANOW,&old);
    printf("\033[2J\033[H");
    return 0;
}
