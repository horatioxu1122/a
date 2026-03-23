/* tmux — one session "a", windows are jobs */
#define TMS "a"
static void tm_ensure_sess(void){(void)!system("tmux has-session -t '"TMS"' 2>/dev/null||tmux new-session -d -s '"TMS"'");}
static int tm_has(const char *w) {
    char c[B];snprintf(c,B,"tmux list-windows -t '"TMS"' -F '#{window_name}' 2>/dev/null|grep -qx '%s'",w);
    return !system(c);
}
static void tm_t(const char*w,char*t){if(*w=='%')snprintf(t,256,"%s",w);else snprintf(t,256,TMS":%s",w);}
static void tm_go(const char *w) {
    perf_disarm();char t[256];tm_t(w,t);
    if(getenv("TMUX"))execlp("tmux","tmux","select-window","-t",t,(char*)NULL);
    else{char c[B];snprintf(c,B,"tmux select-window -t '%s' 2>/dev/null",t);(void)!system(c);
        execlp("tmux","tmux","attach","-t",TMS,(char*)NULL);}
}
static int tm_new(const char *w, const char *wd, const char *cmd) {
    tm_ensure_sess();char c[B*2];
    if(cmd&&*cmd)snprintf(c,sizeof(c),"tmux new-window -t '"TMS":' -n '%s' -c '%s' '%s' \\; set-option -w automatic-rename off",w,wd,cmd);
    else snprintf(c,sizeof(c),"tmux new-window -t '"TMS":' -n '%s' -c '%s' \\; set-option -w automatic-rename off",w,wd);
    return system(c);
}
static void tm_sk(const char*w,const char*s,int l){char t[256];tm_t(w,t);pid_t p=fork();
    if(p==0){if(l)execlp("tmux","tmux","send-keys","-l","-t",t,s,(char*)NULL);
    else execlp("tmux","tmux","send-keys","-t",t,s,(char*)NULL);_exit(1);}
    if(p>0)waitpid(p,NULL,0);}
#define tm_send(w,s) tm_sk(w,s,1)
#define tm_key(w,s) tm_sk(w,s,0)
static int tm_read(const char*w,char*buf,int len){char t[256];tm_t(w,t);
    char c[B];snprintf(c,B,"tmux capture-pane -t '%s' -p 2>/dev/null",t);return pcmd(c,buf,len);}
/* job cmd */
static void jcmd_fill(char*b,int cont,const char*wd){char fp[128]="";{const char*fk=strstr(wd,"/adata/forks/");if(fk)snprintf(fp,128,"a fork run '%s' ",fk+13);}snprintf(b,B,"tmux splitw -vd -p50 -t $TMUX_PANE;while :;do %sclaude --dangerously-skip-permissions%s;e=$?;[ $e -eq 0 ]&&break;echo \"$(date) $e $(pwd)\">>%s/crashes.log;echo \"! crash $e, restarting..\";sleep 2;done;git add -A&&git diff --cached --quiet 2>/dev/null||{ m=$(head -1 .a_done 2>/dev/null||echo done);git commit -m \"job: $m\"&&git push -u origin HEAD 2>/dev/null&&gh pr create --fill 2>/dev/null&&a email \"[a job] $(basename $PWD)\" \"$m\";}",fp,cont?" --continue":"",LOGDIR);}

static void tm_ensure_conf(void) {
    if (strcmp(cfget("tmux_conf"), "y") != 0) return;
    if(fork())return;setsid();
    char adir[P]; snprintf(adir, P, "%s/.a", HOME);
    mkdirp(adir);
    char cpath[P]; snprintf(cpath, P, "%s/tmux.conf", adir);
    FILE *f = fopen(cpath, "w");
    if (!f) return;
    const char *cc = clip_cmd();
    fputs("# aio-managed-config\n"
        "set -ga update-environment \"WAYLAND_DISPLAY\"\n"
        "set -g mouse on\n"
        "set -g focus-events on\n"
        "set -g set-titles on\n"
        "set -g set-titles-string \"#S:#W\"\n"
        "set -s set-clipboard on\n"
        "set -g visual-bell off\n"
        "set -g bell-action any\n"
        "set-hook -g alert-bell 'run-shell \"osascript -e \\\"display notification \\\\\\\"#{hook_window_name}\\\\\\\" with title \\\\\\\"a: done\\\\\\\"\\\"\"'\n"
        "set -g repeat-time 0\n"
        "set -s extended-keys on\n"
        "set -as terminal-features 'xterm*:extkeys'\n"
        "set -g assume-paste-time 0\n"
        "set -g window-style bg=default\n"
        "set -g window-active-style bg=default\n"
        "set -g pane-border-style fg=colour238\n"
        "set -g pane-active-border-style fg=green\n"
        "set -g status-style 'bg=default,fg=white,fill=default'\n"
        "set -g status-position bottom\n"
        "set -g status 2\n"
        "set -g status-right \"\"\n"
        "set -g status-format[0] \"#[align=left]#{?#{e|>:#{session_windows},1},#[range=user|prev]  <  #[norange],}#[align=centre]#{W:#[range=window|#{window_index}]#{?window_bell_flag,#[fg=white bg=red bold],#{?window_active,#[fg=colour232 bg=colour231 bold],#[fg=colour231 bg=colour243]}} #{?window_bell_flag,\\U0001F534 ,}#I:#W #{?window_active, , }#[default]#[norange]}#[align=right]#{?#{e|>:#{session_windows},1},#[range=user|next]  >  #[norange],}\"\n"
        "set -g status-format[1] \"#[align=centre]#[range=user|aa]a#[norange] #[range=user|agent]Agent#[norange] #[range=user|win]Win#[norange] #[range=user|new]Pane#[norange] #[range=user|close]Close#[norange] #[range=user|menu] ... #[norange]#[align=right]#[range=user|kbd]Kb#[norange]\"\n"
        "bind-key -n M-Right next-window\n"
        "bind-key -n M-Left previous-window\n"
        /* C-Tab/C-S-Tab won't work: Tab=0x09=C-i, so C-Tab is indistinguishable from Tab */
        "bind-key -n C-k next-window\n"
        "bind-key -n C-j previous-window\n"
        "bind-key -n C-n new-window\n"
        "bind-key -n C-t new-window\n"
        "bind-key -n C-y split-window -fh\n"
        "bind-key -n C-a run-shell 'tmux new-window \"a\"'\n"
        "bind-key -n C-w kill-window\n"
        "bind-key -n C-q detach\n"
        "bind-key -n C-x kill-session\n"
        "bind-key -T root MouseDown1Status if -F '#{==:#{mouse_status_range},window}' "
        "{ select-window } { run-shell 'r=\"#{mouse_status_range}\"; case \"$r\" in "
        "prev) tmux previous-window;; next) tmux next-window;; "
        "aa) tmux new-window \"a\";; "
        "agent) tmux new-window \"a j a\";; "
        "win) tmux new-window;; new) if [ $(tmux display -p \"#{window_panes}\") -gt 1 ];then tmux kill-pane;else tmux split-window;fi;; "
        "close) tmux kill-window;; "
        "menu) tmux display-menu Pane 1 \"split-window -fh\" Zoom 2 \"resize-pane -Z\" Sync 3 \"set synchronize-panes\" Rename 4 \"command-prompt \\\"rename-window %%\\\"\" Quit 5 detach Kill 6 kill-session;; "
        "kbd) tmux set -g mouse off; tmux display-message \"Mouse off 3s\"; "
        "(sleep 3; tmux set -g mouse on) &;; esac' }\n", f);
    if (access("/data/data/com.termux",F_OK)==0)
        fprintf(f,"set-environment -g CLAUDE_CODE_TMPDIR \"%s/.tmp\"\n",HOME);
    if (cc) fprintf(f, "set -s copy-command \"%s\"\n", cc);
    {const char*cm[]={"copy-mode","copy-mode-vi",NULL};
    for(int i=0;cm[i];i++) cc?fprintf(f,"bind -T %s MouseDragEnd1Pane send -X copy-pipe-and-cancel \"%s\"\n",cm[i],cc)
        :fprintf(f,"bind -T %s MouseDragEnd1Pane send -X copy-pipe-and-cancel\n",cm[i]);}
    char vbuf[64] = ""; int vmaj = 0, vmin = 0;
    pcmd("tmux -V 2>/dev/null", vbuf, 64);
    { char *v = strstr(vbuf, "tmux "); if (v) sscanf(v + 5, "%d.%d", &vmaj, &vmin); }
    if (vmaj > 3 || (vmaj == 3 && vmin >= 6))
        fputs("set -g pane-scrollbars on\nset -g pane-scrollbars-position right\n", f);
    fclose(f);
    char uconf[P]; snprintf(uconf, P, "%s/.tmux.conf", HOME);
    char *uc = readf(uconf, NULL);
    if (!uc || !strstr(uc, "~/.a/tmux.conf")) {
        FILE *uf = fopen(uconf, "a");
        if (uf) { fputs("\nsource-file -q ~/.a/tmux.conf  # a\n", uf); fclose(uf); }
    }
    free(uc);
    {char cmd[B];snprintf(cmd,B,"tmux source-file '%s' 2>/dev/null&&tmux refresh-client -S 2>/dev/null",cpath);(void)!system(cmd);}
    _exit(0);
}
