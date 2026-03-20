#if 0
[ -z "$BASH_VERSION" ] && exec bash "$0" "$@"
set -e; D="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cc -O2 -w -o "$D/betterdash" "$D/betterdash.c"
exec "$D/betterdash" "$@"
#endif
/*
 * betterdash — tmux dashboard
 * Data/command layer separated from visualization.
 * betterdash --list    → parseable session list
 * betterdash --capture <name> → pane content
 * betterdash           → TUI
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

/* ── data layer ── */
#define MAXS 64
#define SLEN 128
#define CAPBUF 8192
typedef struct { char name[SLEN]; char path[SLEN]; char cmd[64]; } sess_t;
static sess_t SES[MAXS];
static int nses;
static char capbuf[CAPBUF];

static void data_load(void) {
	nses = 0;
	FILE *f = popen("tmux list-sessions -F '#{session_name}\t#{pane_current_path}\t#{pane_current_command}' 2>/dev/null", "r");
	if (!f) return;
	char ln[512];
	while (fgets(ln, 512, f) && nses < MAXS) {
		ln[strcspn(ln, "\n")] = 0;
		char *t1 = strchr(ln, '\t'), *t2 = t1 ? strchr(t1+1, '\t') : 0;
		if (t1) *t1 = 0; if (t2) *t2 = 0;
		snprintf(SES[nses].name, SLEN, "%s", ln);
		snprintf(SES[nses].path, SLEN, "%s", t1 ? t1+1 : "");
		snprintf(SES[nses].cmd, 64, "%s", t2 ? t2+1 : "");
		nses++;
	}
	pclose(f);
}

static int data_capture(const char *name) {
	char c[256];
	snprintf(c, 256, "tmux capture-pane -t '=%s' -p 2>/dev/null", name);
	FILE *f = popen(c, "r");
	if (!f) { capbuf[0] = 0; return 0; }
	int n = (int)fread(capbuf, 1, CAPBUF-1, f);
	capbuf[n > 0 ? n : 0] = 0;
	pclose(f);
	return n;
}

/* ── command layer ── */
static void cmd_kill(const char *name) {
	char c[256]; snprintf(c, 256, "tmux kill-session -t '=%s' 2>/dev/null", name);
	FILE *f = popen(c, "r"); if (f) pclose(f);
}

/* ── CLI modes ── */
static int cli_list(void) {
	data_load();
	for (int i = 0; i < nses; i++)
		printf("%s\t%s\t%s\n", SES[i].name, SES[i].path, SES[i].cmd);
	return 0;
}
static int cli_capture(const char *name) {
	data_capture(name);
	printf("%s", capbuf);
	return 0;
}

/* ── terminal layer ── */
#define OBUF 512
static char obuf[OBUF];
static int nobuf;
static struct termios oldtty;
static int nrow, ncol;

static void ttflush(void) {
	if (nobuf) { (void)!write(1, obuf, nobuf); nobuf = 0; }
}
static void ttputc(int c) {
	if (nobuf >= OBUF) ttflush();
	obuf[nobuf++] = c;
}
static void ttputs(const char *s) { while (*s) ttputc(*s++); }
static void ttmove(int r, int c) {
	char b[16]; int n = snprintf(b, 16, "\033[%d;%dH", r+1, c+1);
	for (int i = 0; i < n; i++) ttputc(b[i]);
}
static void tteeol(void) { ttputs("\033[K"); }

static void ttopen(void) {
	struct termios t; struct winsize ws;
	tcgetattr(0, &oldtty); t = oldtty;
	cfmakeraw(&t); tcsetattr(0, TCSADRAIN, &t);
	if (ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_row) {
		nrow = ws.ws_row; ncol = ws.ws_col;
	} else { nrow = 24; ncol = 80; }
}
static void ttclose(void) {
	ttputs("\033[2J\033[H\033[?25h");
	ttflush(); tcsetattr(0, TCSADRAIN, &oldtty);
}
static int ttgetc(void) {
	unsigned char c; return read(0, &c, 1) == 1 ? c : -1;
}

/* ── TUI render ── */
static int sel;
static int preview; /* 1=show capture below list */

static void render(void) {
	int listrows = preview ? nrow / 3 : nrow - 1;
	if (listrows < 1) listrows = 1;
	ttputs("\033[?25l");
	/* session list */
	const char *bname;
	for (int i = 0; i < listrows && i < nses; i++) {
		ttmove(i, 0);
		ttputc(i == sel ? '>' : ' ');
		ttputc(' ');
		/* basename of path */
		bname = strrchr(SES[i].path, '/');
		bname = bname ? bname+1 : SES[i].path;
		char ln[256]; int ll = snprintf(ln, 256, "%-20s %-5s %s", SES[i].name, SES[i].cmd, bname);
		int max = ncol - 3;
		for (int j = 0; j < ll && j < max; j++) ttputc(ln[j]);
		tteeol();
	}
	for (int i = nses; i < listrows; i++) { ttmove(i, 0); tteeol(); }
	/* separator + preview */
	if (preview && nses > 0) {
		ttmove(listrows, 0);
		ttputs("\033[90m");
		for (int i = 0; i < ncol && i < 200; i++) ttputc('-');
		ttputs("\033[0m");
		tteeol();
		/* render capture lines */
		int pstart = listrows + 1, prows = nrow - pstart - 1;
		char *p = capbuf; int row = 0;
		while (row < prows) {
			ttmove(pstart + row, 0);
			char *nl = strchr(p, '\n');
			int len = nl ? (int)(nl - p) : (int)strlen(p);
			int max = ncol;
			for (int j = 0; j < len && j < max; j++) ttputc(p[j]);
			tteeol();
			row++;
			if (!nl || !*p) break;
			p = nl + 1;
		}
		for (; row < prows; row++) { ttmove(pstart + row, 0); tteeol(); }
	}
	/* status */
	ttmove(nrow - 1, 0);
	ttputs("\033[7m j/k p:preview x:kill q \033[0m");
	tteeol();
	ttflush();
}

/* resize */
static volatile sig_atomic_t resized;
static void sigwinch(int s) { (void)s; resized = 1; }

static int tui_run(void) {
	data_load();
	if (preview && nses > 0) data_capture(SES[sel < nses ? sel : 0].name);
	ttopen();
	signal(SIGWINCH, sigwinch);
	sel = 0; render();
	for (;;) {
		if (resized) {
			struct winsize ws;
			if (ioctl(0, TIOCGWINSZ, &ws) == 0) { nrow = ws.ws_row; ncol = ws.ws_col; }
			resized = 0; render();
		}
		int c = ttgetc();
		if (c < 0) break;
		int moved = 0;
		if (c == 'q' || c == 3) break;
		else if (c == 'j' && nses) { sel = sel < nses-1 ? sel+1 : 0; moved = 1; }
		else if (c == 'k' && nses) { sel = sel > 0 ? sel-1 : nses-1; moved = 1; }
		else if (c == 'p') { preview = !preview; if (preview && nses) data_capture(SES[sel].name); render(); continue; }
		else if (c == 'r') { data_load(); if (sel >= nses) sel = nses ? nses-1 : 0; if (preview && nses) data_capture(SES[sel].name); render(); continue; }
		else if (c == 'x' && nses) { cmd_kill(SES[sel].name); data_load(); if (sel >= nses) sel = nses ? nses-1 : 0; if (preview && nses) data_capture(SES[sel].name); render(); continue; }
		else if (c == 27) {
			int c2 = ttgetc(); if (c2 != '[') continue;
			int c3 = ttgetc();
			if (c3 == 'A' && nses) { sel = sel > 0 ? sel-1 : nses-1; moved = 1; }
			else if (c3 == 'B' && nses) { sel = sel < nses-1 ? sel+1 : 0; moved = 1; }
		}
		if (moved) { if (preview && nses) data_capture(SES[sel].name); render(); }
	}
	ttclose();
	return 0;
}

int main(int argc, char **argv) {
	if (argc > 1 && !strcmp(argv[1], "--list")) return cli_list();
	if (argc > 2 && !strcmp(argv[1], "--capture")) return cli_capture(argv[2]);
	return tui_run();
}
