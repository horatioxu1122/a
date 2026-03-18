#if 0
# ── a.c — agent manager. sh a.c [build|install|analyze|shell|clean]
# Polyglot: shell sees # as comments; C preprocessor skips #if 0..#endif.
# Fixes: fewer tokens, same speed+. Features: cut until it breaks.
# Read codebase: a cat (1=all 2=core 3=first10+last5, copies to clipboard)
# TERMUX: set CLAUDE_CODE_TMPDIR=$HOME/.tmp; build with clang directly.
case "$0" in *a.c) [ -z "$BASH_VERSION" ] && exec bash "$0" "$@";; *)
    set -e; A="$HOME/a"
    command -v git >/dev/null || { echo "Install git first"; exit 1; }
    [ -d "$A/.git" ] && { echo "a already installed at $A"; exec sh "$A/a.c" install; }
    git clone https://github.com/seanpattencode/a.git "$A" && exec sh "$A/a.c" install
    exit 1;; esac
set -e
D="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

G='\033[32m' Y='\033[33m' C='\033[36m' R='\033[0m'
ok() { echo -e "${G}✓${R} $1"; }
info() { echo -e "${C}>${R} $1"; }
warn() { echo -e "${Y}!${R} $1"; }

_ensure_cc() {
    CC=$(compgen -c clang- 2>/dev/null|grep -xE 'clang-[0-9]+'|sort -t- -k2 -rn|head -1) || CC=""
    [[ -z "$CC" ]] && command -v clang &>/dev/null && CC=clang
    [[ -n "$CC" ]] && return 0
    info "Installing clang..."
    if [[ -f /data/data/com.termux/files/usr/bin/bash ]]; then pkg install -y clang || true
    elif [[ -f /etc/debian_version ]]; then [[ -f "$D/adata/git/settings/dev" ]] && sudo find /etc/apt/sources.list.d -name '*llvm*' -delete 2>/dev/null; sudo apt-get update -qq && sudo apt-get install -y clang 2>/dev/null || true
    elif [[ -f /etc/arch-release ]]; then sudo pacman -S --noconfirm clang 2>/dev/null || true
    elif [[ -f /etc/fedora-release ]]; then sudo dnf install -y clang 2>/dev/null || true
    elif [[ "$OSTYPE" == darwin* ]]; then xcode-select --install 2>/dev/null; echo "Run 'xcode-select --install' and retry"; exit 1; fi
    command -v clang &>/dev/null && { CC=clang; return 0; }
    command -v gcc &>/dev/null && { warn "using gcc"; CC=gcc; return 0; }
    echo "ERROR: no compiler"; exit 1
}
_warn_flags() {
    if [[ "$CC" == *clang* ]]; then
        WARN="-std=c17 -Werror -Wno-unknown-warning-option -Weverything -Wno-padded -Wno-disabled-macro-expansion -Wno-reserved-id-macro -Wno-documentation -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-used-but-marked-unused -Wno-pre-c11-compat -Wno-implicit-void-ptr-cast -Wno-nullable-to-nonnull-conversion -Wno-poison-system-directories --system-header-prefix=/usr/include -isystem /usr/local/include"
    else WARN="-std=c17 -w"; fi
}
_shell_funcs() {
    for RC in "$HOME/.bashrc" "$HOME/.zshrc"; do
        touch "$RC"
        grep -q '.local/bin' "$RC" 2>/dev/null || echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$RC"
        sed -i.bak '/^_ADD=/d;/^a() {/,/^}/d;/^aio() /d;/^ai() /d;/aios/d' "$RC";rm "$RC.bak"
        echo "_ADD=\"${D%%/adata/worktrees/*}/adata/local\"" >> "$RC"
        cat >> "$RC" << 'AFUNC'
a() {
    local dd="$_ADD"
    local d="${1/#\~/$HOME}"
    [[ -d "$d" ]] && { echo "📂 $d"; cd "$d"; return; }
    [[ "$1" == *.c && -f "$1" ]] && { sh "$@"; return; }
    [[ "$1" == *.py && -f "$1" ]] && { local py=python3 ev=1; [[ -n "$VIRTUAL_ENV" ]] && py="$VIRTUAL_ENV/bin/python" ev=0; [[ -x .venv/bin/python ]] && py=.venv/bin/python ev=0; local s=$(($(date +%s%N)/1000000)); if command -v uv &>/dev/null && [[ -f pyproject.toml || -f uv.lock ]]; then uv run python "$@"; ev=0; else $py "$@"; fi; local r=$?; echo "{\"cmd\":\"$1\",\"ms\":$(($(($(date +%s%N)/1000000))-s)),\"ts\":\"$(date -Iseconds)\"}" >> $dd/timing.jsonl; [[ $r -ne 0 && $ev -ne 0 ]] && printf '  try: a c fix python env for this project\n'; return $r; }
    [[ "$1" == copy && -z "$TMUX" && -t 0 ]] && { local lc; lc=$(fc -ln -2 -2 2>/dev/null); lc=${lc#"${lc%%[! ]*}"}; [[ "$lc" && "$lc" != a\ copy* ]] && { eval "$lc" 2>&1|command a copy; return; }; echo "x No prev cmd"; return 1; }
    command a "$@"; local r=$?; [[ $r -ne 0 && ( "$1" == update || "$1" == u ) ]] && { git -C "${dd%/adata/local}" pull --ff-only 2>/dev/null; sh "${dd%/adata/local}/a.c"; return; }; [[ -f $dd/cd_target ]] && { read -r d < $dd/cd_target; rm $dd/cd_target; cd "$d" 2>/dev/null; }; return $r
}
aio() { a "$@"; }
ai() { a "$@"; }
AFUNC
    done
    if [[ -d /data/data/com.termux ]]; then
        mkdir -p "$HOME/.tmp"
        grep -q CLAUDE_CODE_TMPDIR "$HOME/.bashrc" 2>/dev/null || echo 'export CLAUDE_CODE_TMPDIR="$HOME/.tmp"' >> "$HOME/.bashrc"
        tmux set-environment -g CLAUDE_CODE_TMPDIR "$HOME/.tmp" 2>/dev/null || :
    fi
    ok "shell funcs"
}
_install_node() {
    mkdir -p "$HOME/.local/bin"; export PATH="$HOME/.local/bin:$PATH"
    ARCH=$(uname -m); [[ "$ARCH" == "x86_64" ]] && ARCH="x64"; [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]] && ARCH="arm64"
    if [[ "$OSTYPE" == darwin* ]]; then URL="https://nodejs.org/dist/v22.12.0/node-v22.12.0-darwin-$ARCH.tar.gz"
    else URL="https://nodejs.org/dist/v22.12.0/node-v22.12.0-linux-$ARCH.tar.xz"; fi
    info "Installing node v22.12.0 ($ARCH)..."
    if [[ "$URL" == *.gz ]]; then curl -fsSL "$URL" | tar -xzf - -C "$HOME/.local" --strip-components=1
    else curl -fsSL "$URL" | tar -xJf - -C "$HOME/.local" --strip-components=1; fi
    [[ -x "$HOME/.local/bin/node" ]] && ok "node $($HOME/.local/bin/node -v)" || warn "node install failed"
}
case "${1:-build}" in
node) N="$HOME/.local/bin/node"; [[ -x "$N" ]] && V="$("$N" -v)" && [[ "$V" == v2[2-9]* || "$V" == v[3-9]* ]] && { ok "node $V"; exit 0; }; _install_node ;;
build)
    R="${D%%/adata/worktrees/*}"; if [[ "$D" == *"/adata/worktrees/"* ]]; then ABIN="$D"; else ABIN="$R/adata/local"; fi
    BIN="$HOME/.local/bin";mkdir -p "$ABIN" "$BIN"
    rm -f "$ABIN/.chk"
    printf '%s' $$ > "$ABIN/.bld"
    if [[ -x "$(type -P tcc)" ]]; then
        TCT=${EPOCHREALTIME/./};tcc -DSRC="\"$D\"" -w -o "$ABIN/a" "$D/a.c" 2>/dev/null||exit 1;TCT=$(( ${EPOCHREALTIME/./} - TCT ))000
    else
        _ensure_cc; $CC -DSRC="\"$D\"" -w -O0 -o "$ABIN/a" "$D/a.c" || exit 1
    fi
    [[ "$D" != *"/adata/worktrees/"* ]] && ln -sf "$ABIN/a" "$BIN/a"
    (
        T=$(mktemp -d);trap "rm -rf $T" EXIT;F="$D/a.c";A="-DSRC=\"$D\""
        if command -v tcc >/dev/null && [[ -n "$TCT" ]]; then
            PYT=$(date +%s%N);python3 -c 'import subprocess;subprocess.run(["echo","hello world"],capture_output=True)';PYT=$(( $(date +%s%N)-PYT ))
            [[ $TCT -gt $PYT ]] && { echo "PERF KILL: tcc ${TCT}ns > python ${PYT}ns" >"$ABIN/.chk"; touch "$T/0.f"; }
        fi
        _ensure_cc; _warn_flags
        _c(){ n=$1;shift;{ ! command -v "$1" &>/dev/null||"$@";}>"$T/$n" 2>&1||touch "$T/$n.f";}
        _rgcc(){ command -v gcc &>/dev/null&&! gcc --version 2>&1|grep -q clang;}
        _c 1 $CC $WARN $A -fsyntax-only "$F" &
        _c 3 clang-tidy --checks='-*,bugprone-branch-clone,bugprone-infinite-loop,bugprone-sizeof-*' -warnings-as-errors='*' "$F" -- $A -std=c17 -w &
        { ! _rgcc||gcc -std=c17 -Werror -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wtrampolines $A -fsyntax-only "$F";}>"$T/4" 2>&1||touch "$T/4.f" &
        { ! _rgcc||{ gcc -fanalyzer $A -fsyntax-only "$F" >"$T/5" 2>&1;! grep -q '\-Wanalyzer' "$T/5";};}||touch "$T/5.f" &
        _c 6 cppcheck --error-exitcode=1 --quiet --suppress=syntaxError $A "$F" & _c 7 frama-c -eva -eva-no-print -no-unicode -cpp-extra-args="$A" "$F" &
        { ! command -v cbmc &>/dev/null||timeout 15 cbmc --function main "$F" $A||[ $? -eq 124 ];}>"$T/8" 2>&1||touch "$T/8.f" &
        { $CC $A -fsanitize=undefined,address -fno-omit-frame-pointer -w -o "$T/a.san" "$F"&&A_BENCH=1 "$T/a.san" help >"$T/9" 2>&1;! grep -q 'runtime error\|SUMMARY:.*Sanitizer' "$T/9";}||touch "$T/9.f" &
        wait
        if ls "$T"/[0-9].f &>/dev/null;then cat "$T"/[0-9] >"$ABIN/.chk" 2>/dev/null
            [ "$(cat "$ABIN/.bld" 2>&-)" = "$$" ]&&printf '#!/bin/sh\nhead -80 %s/.chk;exit 1' "$ABIN">"$ABIN/a"&&chmod +x "$ABIN/a"
        else $CC $A -O3 -march=native -flto -w -o "$ABIN/a.opt" "$F"&&[ "$(cat "$ABIN/.bld" 2>&-)" = "$$" ]&&mv "$ABIN/a.opt" "$ABIN/a" 2>&-;rm -f "$ABIN/a.opt"
        fi
    ) >&- 2>&- &
    ;;
analyze) _ensure_cc;_warn_flags
    $CC $WARN -DSRC="\"$D\"" --analyze -Xanalyzer -analyzer-output=text -Xanalyzer -analyzer-checker=security,unix,nullability,optin.portability.UnixAPI -Xanalyzer -analyzer-disable-checker=security.insecureAPI.DeprecatedOrUnsafeBufferHandling "$D/a.c"
    find "$D" -maxdepth 1 -name '*.plist' -delete;;
shell) _shell_funcs;;
clean) rm -f "$D/adata/local/a";;
install)
    BIN="$HOME/.local/bin"; mkdir -p "$BIN"; export PATH="$BIN:$PATH"
    if [[ "$OSTYPE" == darwin* ]]; then OS=mac
    elif [[ -f /data/data/com.termux/files/usr/bin/bash ]]; then OS=termux
    elif [[ -f /etc/debian_version ]]; then OS=debian
    elif [[ -f /etc/arch-release ]]; then OS=arch
    elif [[ -f /etc/fedora-release ]]; then OS=fedora
    else OS=unknown; fi
    SUDO="" NEED_SUDO=0
    { command -v tmux &>/dev/null && command -v npm &>/dev/null && grep -q 'a\.local' /etc/hosts 2>/dev/null; } || NEED_SUDO=1
    if [[ $EUID -eq 0 ]]; then SUDO=""
    elif sudo -n true 2>/dev/null; then SUDO="sudo"
    elif [[ $NEED_SUDO -eq 1 ]] && command -v sudo &>/dev/null && [[ -t 0 ]]; then info "sudo needed for system packages + /etc/hosts"; sudo -v && SUDO="sudo"
    fi
    info "Detected: $OS ${SUDO:+(sudo)}${SUDO:-"(no root)"}"
    if ! grep -q 'a\.local' /etc/hosts 2>/dev/null; then
        if [[ -n "$SUDO" || $EUID -eq 0 ]]; then echo '127.0.0.1 a.local'|$SUDO tee -a /etc/hosts>/dev/null&&ok "a.local"
        elif [[ "$OS" == termux ]]; then ok "a.local (termux: use localhost:1111)"
        else warn "a.local: run 'echo 127.0.0.1 a.local | sudo tee -a /etc/hosts'"; fi
    else ok "a.local"; fi
    install_node() { local v;v=$(node -v 2>/dev/null)&&[[ "$v" == v2[2-9]*||"$v" == v[3-9]* ]]&&return 0;_install_node; }
    case $OS in
        mac)
            command -v brew &>/dev/null || { info "Installing Homebrew..."; /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; eval "$(/opt/homebrew/bin/brew shellenv 2>/dev/null || /usr/local/bin/brew shellenv)"; }
            brew tap hudochenkov/sshpass 2>/dev/null; brew install tmux node gh sshpass rclone cppcheck gcc &>/dev/null||brew upgrade tmux node gh sshpass rclone cppcheck gcc &>/dev/null
            command -v clang &>/dev/null || { xcode-select --install 2>/dev/null; warn "Run 'xcode-select --install' then retry"; }
            ok "tmux + node + gh + rclone" ;;
        debian)
            if [[ -n "$SUDO" ]]; then export DEBIAN_FRONTEND=noninteractive
                $SUDO apt update -qq && $SUDO apt install -yqq clang tmux git curl python3-pip sshpass rclone tcc gcc cppcheck cbmc frama-c-base 2>/dev/null || true
                command -v gh &>/dev/null||{ curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg|$SUDO tee /etc/apt/keyrings/gh.gpg>/dev/null&&echo "deb [signed-by=/etc/apt/keyrings/gh.gpg] https://cli.github.com/packages stable main"|$SUDO tee /etc/apt/sources.list.d/gh.list>/dev/null&&$SUDO apt update -qq&&$SUDO apt install -yqq gh;}||true; ok "pkgs"
            fi; install_node; [[ -z "$SUDO" ]] && { command -v tmux &>/dev/null || warn "tmux needs: sudo apt install tmux"; } ;;
        arch)
            if [[ -n "$SUDO" ]]; then $SUDO pacman -Sy --noconfirm clang tmux nodejs npm git python-pip sshpass rclone github-cli tcc gcc cppcheck cbmc frama-c 2>/dev/null && ok "pkgs"
            else install_node; command -v tmux &>/dev/null || warn "tmux needs: sudo pacman -S tmux"; fi ;;
        fedora)
            if [[ -n "$SUDO" ]]; then $SUDO dnf install -y clang tmux nodejs npm git python3-pip sshpass rclone gh tcc gcc cppcheck cbmc frama-c 2>/dev/null && ok "pkgs"
            else install_node; command -v tmux &>/dev/null || warn "tmux needs: sudo dnf install tmux"; fi ;;
        termux) pkg update -y && pkg upgrade -y -o Dpkg::Options::=--force-confold && pkg install -y build-essential tcc tmux nodejs git python openssh sshpass gh rclone cronie termux-services && mkdir -p ~/.gyp && echo "{'variables':{'android_ndk_path':''}}" > ~/.gyp/include.gypi && ok "pkgs" ;;
        *) install_node; warn "Unknown OS - install tmux manually" ;;
    esac
    _ensure_cc
    sh "$D/a.c" && ok "a compiled" || warn "Build failed"
    E="$HOME/editor"
    [[ -f "$E/e.c" ]] || git clone https://github.com/seanpattencode/editor "$E" 2>/dev/null || :
    [[ -f "$E/e.c" ]] && sh "$E/e.c" install || :
    _shell_funcs
    install_cli() {
        local pkg="$1" cmd="$2" p=$(command -v "$cmd" 2>/dev/null)
        [[ -n "$p" && "${p:0:5}" != "/mnt/" ]] && { ok "$cmd"; return; }
        [[ -n "$p" ]] && warn "$cmd ($p) is Windows"; info "Installing $cmd..."
        local ns=""; [[ "$OS" == termux && -z "$3" ]] && ns="--ignore-scripts"
        if ! command -v npm &>/dev/null; then warn "$cmd skipped (no npm)"
        elif [[ -n "$SUDO" || $EUID -eq 0 ]]; then $SUDO npm install -g $ns "$pkg"&&ok "$cmd"||warn "$cmd failed"
        else mkdir -p "$HOME/.local/lib"&&npm install -g $ns --prefix="$HOME/.local" "$pkg"&&ok "$cmd"||warn "$cmd failed"; fi
    }
    command -v claude &>/dev/null&&ok "claude"||{ info "Installing claude...";curl -fsSL https://claude.ai/install.sh|bash&&ok "claude"||warn "claude failed";}
    install_cli "@openai/codex" "codex"
    install_cli "@google/gemini-cli" "gemini" scripts
    [[ "$OS" == termux ]] && info "Gemini auth: NO_BROWSER=true gemini"
    command -v uv &>/dev/null&&ok "uv"||{ info "Installing uv...";curl -LsSf https://astral.sh/uv/install.sh|sh&&export PATH="$HOME/.local/bin:$PATH"&&ok "uv"||warn "uv failed";}
    if ! command -v uv &>/dev/null; then
        _best_py(){ for v in python3.14 python3.13 python3.12 python3.11 python3;do command -v $v &>/dev/null&&$v -c 'import venv' 2>/dev/null&&echo $v&&return;done;}
        VENV="$D/adata/venv";PY=$(_best_py)
        [[ -n "$PY" ]]&&{ [[ -f "$VENV/bin/python" ]]&&ok "venv"||{ $PY -m venv "$VENV"&&ok "venv"||warn "venv failed";}
        [[ -f "$VENV/bin/pip" ]]&&$VENV/bin/pip install -q pexpect prompt_toolkit aiohttp 2>/dev/null&&ok "python deps"||warn "pip failed";}
    fi
    if ! python3 -c "from playwright.sync_api import sync_playwright; p=sync_playwright().start(); p.chromium.launch(headless=True).close(); p.stop()" 2>/dev/null; then
        info "Installing playwright browser deps..."
        if command -v pacman &>/dev/null; then sudo pacman -S --noconfirm --needed libxcomposite gtk3 alsa-lib nss 2>/dev/null && ok "playwright deps" || warn "playwright deps"
        elif command -v apt-get &>/dev/null; then sudo apt-get install -y libxcomposite1 libgtk-3-0t64 libasound2t64 libnss3 2>/dev/null && ok "playwright deps" || warn "playwright deps"; fi
    else ok "playwright deps"; fi
    command -v ollama &>/dev/null&&ok "ollama"||{ [[ -n "$SUDO" || $EUID -eq 0 ]]&&{ info "Installing ollama...";curl -fsSL https://ollama.com/install.sh|sh&&ok "ollama"||warn "ollama failed";}||warn "ollama needs sudo";}
    "$BIN/a" ui on 2>/dev/null && ok "UI service (localhost:1111)" || :
    [[ ! -s "$HOME/.tmux.conf" ]] && "$BIN/a" config tmux_conf y 2>/dev/null && ok "tmux config (mouse enabled)" || :
    "$BIN/a" >/dev/null 2>&1 && ok "cache generated" || :
    command -v gh &>/dev/null && { gh auth status &>/dev/null || { [[ -t 0 ]] && info "GitHub login enables sync" && read -p "Login? (y/n): " yn && [[ "$yn" =~ ^[Yy] ]] && gh auth login && gh auth setup-git; }; gh auth status &>/dev/null && ok "sync configured"; } || :
    AROOT="$D/adata"; SROOT="$AROOT/git"
    if [[ ! -d "$SROOT/.git" ]]; then mkdir -p "$AROOT"
        { command -v gh &>/dev/null&&gh auth status &>/dev/null 2>&1&&gh repo clone seanpattencode/a-git "$SROOT" 2>/dev/null&&ok "adata/git cloned";}||{ git init -q "$SROOT" 2>/dev/null;ok "adata/git init";}
    fi
    RC="$HOME/.bashrc"; [[ -n "$ZSH_VERSION" ]] && RC="$HOME/.zshrc"
    source "$RC" 2>/dev/null && ok "shell ready" || warn "run: source $RC"
    echo -e "\n${G}✓${R} Install complete — type ${G}a${R}\n"
    ;;
*)
    echo "Usage: sh a.c [build|install|analyze|shell|clean]"
    ;;
esac
exit 0
#endif
/* a.c — self-compiling agent manager. sh a.c && a <args> to test. */
#ifndef __APPLE__
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <limits.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#define P 1024
#define B 4096
#define MP 256
#define MA 64
#define MS 48
#define AB if(getenv("A_BENCH"))return 0
#define CWD(w) char w[P];if(!getcwd(w,P))snprintf(w,P,"%s",HOME)

/* amalgamation */
static void mkdirp(const char *p);
static void alog(const char *cmd, const char *cwd);
static void perf_disarm(void);
static int cmd_sess(int, char**);

#include "lib/globals.c"
#include "lib/init.c"
#include "lib/util.c"
#include "lib/kv.c"
#include "lib/data.c"
#include "lib/tmux.c"
#include "lib/git.c"
#include "lib/session.c"
#include "lib/alog.c"
#include "lib/help.c"
#include "lib/project.c"
#include "lib/config.c"
#include "lib/push.c"
#include "lib/hub.c"
#include "lib/ls.c"
#include "lib/note.c"
#include "lib/ssh.c"
#include "lib/net.c"
#include "lib/cal.c"
#include "lib/agent.c"
#include "lib/perf.c"
#include "lib/sess.c"

static int cmd_freq(int c,char**v){perf_disarm();
    int verbose=0,n=25;
    for(int i=2;i<c;i++){if(!strcmp(v[i],"-v"))verbose=1;else if(*v[i]>='0'&&*v[i]<='9')n=atoi(v[i]);}
    char ad[P];snprintf(ad,P,"%s/git/activity",AROOT);
    DIR*d=opendir(ad);if(!d){puts("x no activity log");return 1;}
    struct{char n[64];int c;}ct[1024];int nc=0;
    struct dirent*e;char fp[P],ln[256];
    while((e=readdir(d))){if(e->d_name[0]=='.')continue;
        snprintf(fp,P,"%s/%s",ad,e->d_name);
        int fd=open(fp,O_RDONLY);if(fd<0)continue;
        int r=(int)read(fd,ln,255);close(fd);if(r<=0)continue;ln[r]=0;
        char*p=ln;for(int i=0;i<3&&*p;i++){while(*p&&*p!=' ')p++;while(*p==' ')p++;}
        char*end=p;while(*end&&*end!='\n')end++;*end=0;
        if(!*p)continue;
        end=p;while(*end&&*end!=' ')end++;
        if(verbose&&*end){end++;while(*end&&*end!=' '&&*end!='\n')end++;}
        *end=0;
        int j;for(j=0;j<nc;j++)if(!strcmp(ct[j].n,p)){ct[j].c++;break;}
        if(j==nc&&nc<1024){snprintf(ct[nc].n,64,"%s",p);ct[nc].c=1;nc++;}}
    closedir(d);
    for(int i=0;i<nc-1;i++)for(int j=i+1;j<nc;j++)if(ct[j].c>ct[i].c){char tn[64];int tc=ct[i].c;memcpy(tn,ct[i].n,64);ct[i].c=ct[j].c;memcpy(ct[i].n,ct[j].n,64);ct[j].c=tc;memcpy(ct[j].n,tn,64);}
    if(n>nc)n=nc;
    for(int i=0;i<n;i++)printf("%6d %s\n",ct[i].c,ct[i].n);
    puts("\033[33m! counts include bot/automated use\033[0m");
    return 0;}
static int cmd_cat(int c,char**v){perf_disarm();
    char m=0;int di=2;
    if(c>2&&v[2][0]>='1'&&v[2][0]<='3'&&!v[2][1]){m=v[2][0];di=3;}
    if(c>di&&chdir(v[di]))return 1;
    if(!m){puts("1 all files, all lines\n2 all files, all lines (skip lab/)\n3 all files, first 10 lines + last 5 lines (skip lab/)");
        printf("> ");fflush(stdout);char ch[4];if(!fgets(ch,4,stdin))return 0;m=ch[0];}
    const char*ex=m!='1'?" -- ':!lab/'":"";
    if(m=='3'){char cm[B];snprintf(cm,B,"git ls-files -z%s|xargs -0 grep -lIZ ''",ex);
        size_t l=0,cap=0;char*d=NULL,b[8192];size_t n;int nf=0,skf=0;
        FILE*fl=popen(cm,"r");char fb[65536];size_t fl2=0;
        if(fl){while((n=fread(b,1,8192,fl))>0){if(fl2+n<65536){memcpy(fb+fl2,b,n);fl2+=n;}}pclose(fl);}
        fb[fl2]=0;
        for(char*p=fb;p<fb+fl2;){char*e=memchr(p,0,(size_t)(fb+fl2-p));if(!e)break;
            if(l>131072&&strchr(p,'/')){skf++;p=e+1;continue;}
            FILE*f=fopen(p,"r");if(!f){p=e+1;continue;}
            int tl=0;char ln[512];while(fgets(ln,512,f))tl++;
            rewind(f);nf++;
            char hdr[600];size_t hl=(size_t)snprintf(hdr,600,"\n==> %s (%d lines) <==\n",p,tl);
            if(l+hl>=cap){cap=(l+hl+8192)*2;d=realloc(d,cap);}memcpy(d+l,hdr,hl);l+=hl;
            int hd=strchr(p,'/')?10:1000;
            int i=0;while(fgets(ln,512,f)){
                size_t sl=strlen(ln);
                if(i<hd||(tl>hd+5&&i>=tl-5)){if(l+sl>=cap){cap=(l+sl+8192)*2;d=realloc(d,cap);}memcpy(d+l,ln,sl);l+=sl;}
                if(i==hd&&tl>hd+5){const char*dots="  ...\n";size_t dl=6;if(l+dl>=cap){cap=(l+dl+8192)*2;d=realloc(d,cap);}memcpy(d+l,dots,dl);l+=dl;}
                i++;}
            fclose(f);p=e+1;}
        if(!d)return 1;d[l]=0;
        (void)!write(1,d,l);to_clip(d);
        if(skf)fprintf(stderr,"✓ %d files %zub (%d skipped)\n",nf,l,skf);
        else fprintf(stderr,"✓ %d files %zub\n",nf,l);
        free(d);return 0;}
    char cm[B];snprintf(cm,B,"git ls-files -z%s|xargs -0 grep -lIZ ''|xargs -0 tail -n+1",ex);
    size_t l=0;char*d=NULL,b[8192];size_t n,cap=0;int nf=0;
    FILE*f=popen(cm,"r");if(f){while((n=fread(b,1,8192,f))>0){
        if(l+n>=cap){cap=(l+n)*2;d=realloc(d,cap+1);}memcpy(d+l,b,n);l+=n;}pclose(f);}
    if(!d)return 1;d[l]=0;for(char*p=d;(p=strstr(p,"==> "));p+=4)nf++;
    (void)!write(1,d,l);to_clip(d);fprintf(stderr,"✓ %d files %zub\n",nf,l);free(d);return 0;}
static int cmd_j(int,char**);
static int cmd_job(int c,char**v){
    if(c>2&&*v[2]>='0'&&*v[2]<='9')return cmd_jobs(c,v);
    return cmd_j(c,v);}
static int cmd_j(int c,char**v){
    if(c<3||!strcmp(v[2],"rm")||!strcmp(v[2],"watch")||!strcmp(v[2],"-r")||(c==3&&isdigit(*v[2])))return cmd_jobs(c,v);
    if(c>2&&v[2][1]=='q'){char ln[B];for(fputs("j> ",stdout);fgets(ln,B,stdin);fputs("j> ",stdout)){
        ln[strcspn(ln,"\n")]=0;if(!*ln)continue;char*a[]={"a","j","--no-wt",ln,0};cmd_j(4,a);}return 0;}
    if(c==3&&!strcmp(v[2],"a")){if(!getenv("TMUX")){puts("x Needs tmux");return 1;}
        char cf[P],pr[B],cm[B],pid[64];snprintf(cf,P,"%s/job_context.txt",DDIR);
        {char nd[P];FILE*f=fopen(cf,"w");if(f){
            snprintf(nd,P,"%s/notes",SROOT);int nn=load_notes(nd,NULL);
            for(int i=0;i<nn;i++)fprintf(f,"%d. %s\n",i+1,gn[i].t);
            fputs("\n",f);snprintf(nd,P,"%s/tasks",SROOT);int nt=load_tasks(nd);
            for(int i=0;i<nt;i++)fprintf(f,"%d. P%s %s\n",i+1,T[i].p,T[i].t);fclose(f);}}
        snprintf(pr,B,"%s/common/prompts/job.txt",SROOT);
        char*ap=readf(pr,NULL);snprintf(pr,B,"%s\nContext: cat %s",ap?ap:"Ask what to work on. cat a.c for source.",cf);if(ap)free(ap);
        snprintf(cm,B,"tmux split-window -fhP -F '#{pane_id}' -c '%s' 'claude --dangerously-skip-permissions'",SDIR);
        pcmd(cm,pid,64);pid[strcspn(pid,"\n")]=0;
        if(pid[0])send_prefix_bg(pid,"claude",SDIR,pr);return 0;}
    {char nb[16]="";pcmd("pgrep -xc claude 2>/dev/null||echo 0",nb,16);
    int nj=atoi(nb)-1;if(nj<0)nj=0;
    if(nj>=100&&!(c>2&&!strcmp(v[2],"--resume"))){printf("x %d/100 job slots full — use 'a job' to see running\n",nj);return 1;}}
    init_db();load_cfg();load_proj();CWD(wd);
    if(c>3&&!strcmp(v[2],"--resume")){snprintf(wd,P,"%s",v[3]);
        if(!dexists(wd)){printf("x %s not found\n",wd);return 1;}
        printf("+ resume: %s\n",wd);
        tm_ensure_conf();char jcmd[B];jcmd_fill(jcmd,1);
        if(!getenv("TMUX")){char sn[64];snprintf(sn,64,"j-%s",bname(wd));tm_new(sn,wd,jcmd);tm_go(sn);}
        else{char cm[B],pid[64];snprintf(cm,B,"tmux new-window -P -F '#{pane_id}' -c '%s' '%s'",wd,jcmd);pcmd(cm,pid,64);}
        return 0;}
    int si=2,nowt=0;if(c>3&&v[2][0]>='0'&&v[2][0]<='9'){int idx=atoi(v[2]);if(idx<NPJ)snprintf(wd,P,"%s",PJ[idx].path);si++;}
    char pr[B]="";int pl=0;for(int i=si;i<c;i++){if(!strcmp(v[i],"--no-wt")){nowt=1;continue;}pl+=snprintf(pr+pl,(size_t)(B-pl),"%s%s",pl?" ":"",v[i]);}
    if(!nowt&&git_in_repo(wd)){
        char wt[P];{const char*w=cfget("worktrees_dir");snprintf(wt,P,"%s%s",w[0]?w:AROOT,w[0]?"":"/worktrees");}
        time_t now=time(NULL);struct tm*t=localtime(&now);char ts[16];
        strftime(ts,16,"%b%d",t);for(char*p=ts;*p;p++)*p=(*p>='A'&&*p<='Z')?*p+32:*p;
        int h=t->tm_hour%12;if(!h)h=12;char nm[64],wp[P],gc[B];
        snprintf(nm,64,"%s-%s-%d%02d%02d%s",bname(wd),ts,h,t->tm_min,t->tm_sec,t->tm_hour>=12?"pm":"am");
        snprintf(wp,P,"%s/%s",wt,nm);
        snprintf(gc,B,"mkdir -p '%s'&&git -C '%s' worktree add -b 'j-%s' '%s' HEAD 2>/dev/null",wt,wd,nm,wp);
        if(!system(gc)){char sl[B];snprintf(sl,B,"ln -s '%s' '%s/adata' 2>/dev/null",AROOT,wp);(void)!system(sl);
            printf("+ %s\n",wp);snprintf(wd,P,"%s",wp);}
    }
    printf("+ job: %s\n  %.*s\n",bname(wd),80,pr);
    if(pr[0])pl+=snprintf(pr+pl,(size_t)(B-pl),"\n\nWhen done: git add -A && git commit -m 'job: <summary>', then write .a_done with summary + test commands");
    tm_ensure_conf();
    char jcmd[B];jcmd_fill(jcmd,0);
    if(!getenv("TMUX")){char sn[64];snprintf(sn,64,"j-%s",bname(wd));
        tm_new(sn,wd,jcmd);send_prefix_bg(sn,"claude",wd,pr);tm_go(sn);}
    char cm[B],pid[64];
    snprintf(cm,B,"tmux new-window -d -n '%s' -P -F '#{pane_id}' -c '%s' '%s'",bname(wd),wd,jcmd);
    pcmd(cm,pid,64);pid[strcspn(pid,"\n")]=0;if(pid[0])send_prefix_bg(pid,"claude",wd,pr);
    return 0;}
static int cmd_adb(int c,char**v){
    if(c>2&&!strcmp(v[2],"ssh"))return system("for s in $(adb devices|awk '/\\tdevice$/{print$1}');do printf '\\033[36m→ %s\\033[0m ' \"$s\";adb -s \"$s\" shell 'am broadcast -n com.termux/.app.TermuxOpenReceiver -a com.termux.RUN_COMMAND --es com.termux.RUN_COMMAND_PATH /data/data/com.termux/files/usr/bin/sshd --ez com.termux.RUN_COMMAND_BACKGROUND true' 2>&1|tail -1;done");
    execlp("adb","adb","devices","-l",(char*)0);return 1;
}
static int cmd_run_once(int c,char**v){
    if(c<3){puts("Usage: a once [-t secs] [claude flags] prompt words...");return 1;}
    unsigned tl=600;int si=2;
    if(c>3&&!strcmp(v[2],"-t")){tl=(unsigned)atoi(v[3]);si=4;}
    perf_disarm();unsetenv("CLAUDECODE");unsetenv("CLAUDE_CODE_ENTRYPOINT");
    char*flags[16];int nf=0;char pr[B]="";int pl=0;
    for(int i=si;i<c;i++){
        if(v[i][0]=='-'&&nf<14){flags[nf++]=v[i];
            if((!strcmp(v[i],"--model")||!strcmp(v[i],"--max-budget-usd"))&&i+1<c)flags[nf++]=v[++i];
        }else pl+=snprintf(pr+pl,(size_t)(B-pl),"%s%s",pl?" ":"",v[i]);}
    char*a[22];int n=0;
    a[n++]="claude";a[n++]="-p";a[n++]="--dangerously-skip-permissions";a[n++]="--model";a[n++]="opus";
    for(int i=0;i<nf;i++)a[n++]=flags[i];
    a[n++]=pr;a[n]=NULL;
    pid_t ch=fork();
    if(ch==0){execvp("claude",a);perror("claude");_exit(127);}
    int st;
    for(unsigned elapsed=0;elapsed<tl;elapsed++){
        pid_t r=waitpid(ch,&st,WNOHANG);
        if(r>0)return WIFEXITED(st)?WEXITSTATUS(st):1;
        sleep(1);}
    fprintf(stderr,"\n\033[31m✗ TIMEOUT\033[0m: a once >%us\n",tl);
    kill(ch,SIGKILL);waitpid(ch,NULL,0);return 124;}
static int cmd_my(int c,char**v){(void)c;(void)v;char d[P];snprintf(d,P,"%s/my",SROOT);
    execlp("ls","ls","--color",d,(char*)0);return 1;}
typedef struct { const char *n; int (*fn)(int, char**); } cmd_t;
static int cmd_cmp(const void*a,const void*b){return strcmp(((const cmd_t*)a)->n,((const cmd_t*)b)->n);}
/* dispatch: C logic+aliases here; lib .py and my scripts auto-discovered.
   TUI (a i) shows every command you can type — it must let you do anything
   you can do without it. gen_icache() is the single source for TUI entries. */
static const cmd_t CMDS[] = {
    {"--help",cmd_help_full},{"-h",cmd_help_full},
    {"a",cmd_a_default},{"adb",cmd_adb},{"add",cmd_add},{"agent",cmd_agent},{"ai",cmd_all},
    {"all",cmd_all},  /* apk,ask,attach,cleanup auto-discovered */
    {"cal",cmd_cal},{"cat",cmd_cat},{"config",cmd_config},
    {"copy",cmd_copy},{"create",cmd_create},{"dash",cmd_dash},{"deps",cmd_deps},
    {"diff",cmd_diff},{"dir",cmd_dir},{"docs",cmd_docs},{"done",cmd_done},
    {"e",cmd_e},{"email",cmd_email},{"freq",cmd_freq},
    {"help",cmd_help_full},{"hi",cmd_hi},{"hub",cmd_hub},{"i",cmd_i},
    {"install",cmd_install},{"j",cmd_j},{"job",cmd_job},{"jobs",cmd_job},
    {"kill",cmd_kill},{"log",cmd_log},{"login",cmd_login},{"ls",cmd_ls},
    {"mono",cmd_cat},{"monolith",cmd_cat},{"move",cmd_move},{"my",cmd_my},
    {"n",cmd_note},{"note",cmd_note},{"once",cmd_run_once},
    {"p",cmd_push},{"perf",cmd_perf},{"pr",cmd_pr},{"prompt",cmd_prompt},
    {"pull",cmd_pull},{"push",cmd_push},
    {"remove",cmd_remove},{"repo",cmd_create},{"revert",cmd_revert},{"review",cmd_review},
    {"rm",cmd_remove},{"run",cmd_run},{"scan",cmd_scan},{"send",cmd_send},
    {"set",cmd_set},{"settings",cmd_settings},{"setup",cmd_setup},
    {"ssh",cmd_ssh},{"ssh add",cmd_ssh},{"ssh all",cmd_ssh},{"ssh rm",cmd_ssh},
    {"ssh self",cmd_ssh},{"ssh setup",cmd_ssh},{"ssh start",cmd_ssh},{"ssh stop",cmd_ssh},
    {"sync",cmd_sync},{"t",cmd_task},{"task",cmd_task},
    {"tree",cmd_tree},{"u",cmd_update},  /* ui auto-discovered */
    {"uninstall",cmd_uninstall},{"update",cmd_update},{"watch",cmd_watch},{"web",cmd_web},
    /* work auto-discovered */
    {"x",cmd_x},
};
#define NCMDS (sizeof(CMDS)/sizeof(*CMDS))
static char perf_msg[B];
__attribute__((noreturn)) static void perf_alarm(int sig){(void)sig;
    (void)!write(STDERR_FILENO,perf_msg,strlen(perf_msg));kill(0,SIGTERM);_exit(124);}
static void perf_arm(const char *cmd) {
    if (getenv("A_BENCH")) return;
    if (isdigit(*cmd)) return;
    {char sk[64];snprintf(sk,64,"|%s|",cmd);if(strstr("|push|pull|sync|u|update|login|ssh|gdrive|mono|cat|email|install|send|j|job|pr|hub|create|repo|e|revert|",sk))return;}
    unsigned secs = 1, limit_us = 1000000;
    char pf[P]; snprintf(pf, P, "%s/perf/%s.txt", SROOT, DEV);
    {char *data = readf(pf, NULL);
    unsigned pl = perf_limit(data, cmd);
    if (pl > 0) { limit_us = pl; secs = (pl + 999999) / 1000000; }
    free(data);}
    snprintf(perf_msg, B,
        "\n\033[31m✗ PERF KILL\033[0m: 'a %s' >%us (%uus, %s)\n  %s\n", cmd, secs, limit_us, DEV, pf);
    signal(SIGALRM, perf_alarm);
    alarm(secs);
}
static void perf_disarm(void) { alarm(0); signal(SIGALRM, SIG_DFL); }
static struct timespec gt0;
static void gt_print(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    long ms=(t.tv_sec-gt0.tv_sec)*1000L+(t.tv_nsec-gt0.tv_nsec)/1000000;
    struct stat st;char p[P];DIR*d;struct dirent*e;
    snprintf(p,P,"%s/a.c",SDIR);long ac=!stat(p,&st)?st.st_size/4:0;
    long dt[2]={0,0};const char*dn[]={"lib","lab"};
    for(int i=0;i<2;i++){snprintf(p,P,"%s/%s",SDIR,dn[i]);d=opendir(p);if(d){
        while((e=readdir(d))){if(e->d_name[0]=='.')continue;snprintf(p,P,"%s/%s/%s",SDIR,dn[i],e->d_name);
            if(!stat(p,&st)&&S_ISREG(st.st_mode))dt[i]+=st.st_size;}closedir(d);}}
    fprintf(stderr,"%ldms tokens a.c:%ld lib:%ld lab:%ld\n",ms,ac,dt[0]/4,dt[1]/4);}
int main(int argc, char **argv) {
    init_paths();G_argc=argc;G_argv=argv;

    if (argc < 2) return (isatty(1)?cmd_i:cmd_help)(argc, argv);

    clock_gettime(CLOCK_MONOTONIC,&gt0);atexit(gt_print);
    char acmd[B]="";ajoin(acmd,B,argc,argv,1);
    CWD(wd);
    alog(acmd, wd);

    const char *arg = argv[1];

    if (*arg && !arg[strspn(arg,"0123456789")]) { init_db(); return cmd_project_num(argc, argv, atoi(arg)); }

    perf_arm(arg);

    { cmd_t key = {arg, NULL};
      const cmd_t *c = bsearch(&key, CMDS, NCMDS, sizeof(*CMDS), cmd_cmp);
      if (c) return c->fn(argc, argv); }

    /* auto-discover lib .py, lib pkg, and lab .py */
    {char pf[P];snprintf(pf,P,"%s/lib/%s.py",SDIR,arg);
     if(fexists(pf))fallback_py(arg,argc,argv);
     snprintf(pf,P,"%s/lib/%s/__init__.py",SDIR,arg);
     if(fexists(pf)){char m[P];snprintf(m,P,"%s/__init__",arg);fallback_py(m,argc,argv);}
     const char*la=(*arg=='x'&&arg[1]=='.')?arg+2:arg;
     snprintf(pf,P,"%s/lab/%s.py",SDIR,la);
     if(fexists(pf)){perf_disarm();argv[1]=pf;argv[0]="python3";execvp("python3",argv);}
     snprintf(pf,P,"%s/lab/%s.c",SDIR,la);
     if(fexists(pf)){perf_disarm();char ob[P],cm[B];snprintf(ob,P,"%s/lab_%s",TMP,la);
      snprintf(cm,B,"cc -w -o '%s' '%s'&&'%s'",ob,pf,ob);return system(cm);}
     snprintf(pf,P,"%s/lab/%s.sh",SDIR,la);
     if(fexists(pf)){perf_disarm();argv[1]=pf;argv[0]="sh";execvp("sh",argv);}
     snprintf(pf,P,"%s/lab/%s.html",SDIR,la);
     if(fexists(pf)){perf_disarm();execlp("xdg-open","xdg-open",pf,(char*)0);}}
    {size_t l=strlen(arg);if(l>=3&&arg[l-1]=='+'&&arg[l-2]=='+'&&*arg!='w')return cmd_wt_plus(argc,argv);}
    if(*arg=='w'&&arg[1]&&!fexists(arg))return cmd_wt(argc,argv);
    {init_db();load_cfg();load_sess();if(find_sess(arg))return cmd_sess(argc,argv);}
    if(dexists(arg)||fexists(arg))return cmd_dir_file(argc,argv);
    {char ep[P];snprintf(ep,P,"%s%s",HOME,arg);if(*arg=='/'&&dexists(ep))return cmd_dir_file(argc,argv);}
    if(strlen(arg)<=3&&islower(*arg))return cmd_sess(argc,argv);
    if(tm_has(arg)){tm_go(arg);return 0;}
    {char mf[P];static const char*X[]={"",".py",".c",".sh",".html",0};
     for(int i=0;X[i];i++){snprintf(mf,P,"%s/my/%s%s",SROOT,arg,X[i]);
      if(fexists(mf)){perf_disarm();char md[P];snprintf(md,P,"%s/my",SROOT);(void)!chdir(md);argv[1]=mf;return cmd_dir_file(argc,argv);}}}
    fprintf(stderr,"a: unknown '%s'\n",arg);
    return 1;
}
/* Repo struture: 
adata: all persistent data
lab: experimental work
lib: flat program files
my: symlink user program files
a.c: command dispatch, install, env setup
AGENTS.md: custom ai instructions
IDEAS.md: explain why project exists
README.md: new user information
*/
