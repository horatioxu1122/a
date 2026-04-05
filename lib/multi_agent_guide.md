# Multi-Agent Guide
Spawn: `a ssh <device> 'cd <dir> && a j "<prompt>"'` — runs in tmux window, no forks.
View: `a ssh <device> 'tmux capture-pane -t a:<win>.0 -p -S -30 | tail -20'`
Health: `a ssh <device> 'free -h|head -2; pgrep -c claude'` Kill: `tmux send-keys -t a:<win>.0 C-c`
## Rules
1. Write YOUR files only, distinct names. 30 lines max per task. Do NOT push — human pushes.
2. Over 100 lines: stop, call human. Rewriting within 30-line tasks is fine.
3. Write .a_done when finished. Do NOT modify a.c, lib/, or other agents' files.
4. All agents share one tmux session, different windows. Read other agents' files, don't message them.
5. Separate devices > local forks. Forks share kernel/GPU/RAM. Network is the real boundary.
