# Multi-Agent Multi-Device Guide

## Spawning agents on another device
```bash
a ssh <device> 'cd ~/aicombo && a j "<prompt>"'
```
No fork or worktrees by default should be used. Agent runs in tmux window on target device.

## Viewing and controlling remote agents
```bash
# List all sessions/windows
a ssh <device> 'tmux list-windows -a'

# Read agent output
a ssh <device> 'tmux capture-pane -t a:<window>.0 -p -S -30 | tail -20'

# Check system health
a ssh <device> 'free -h | head -2; nvidia-smi --query-gpu=memory.used --format=csv,noheader; pgrep -c claude'

# Kill a runaway agent
a ssh <device> 'tmux send-keys -t a:<window>.0 C-c'
```

## Physical device isolation > local isolation
- Forks share kernel, GPU, RAM, /tmp — one agent's OOM kills all
- Separate machines: agent can destroy its box, others survive
- adata syncs via git — nuke and restore in minutes
- Network is the security boundary, not a directory

## Rules for agents working in parallel
1. Write to YOUR files only. Name files distinctly (e.g., awq_analysis.py, not test.py)
2. Do NOT push. Human reviews and pushes
3. Small tasks, frequent intervention. 30 lines max per script per agent work direction given again, and the human should be called to review esp around 100 lines or more before proceeding with that work, but operating and rewriting within those limits is ok.
4. Write .a_done when finished with summary + test command
5. Do NOT modify a.c, lib/, or other agents' files
6. All agents share one tmux session, different windows. Human switches between them
7. If you need another agent's output, read their file — don't message them

## Tmux window layout
All agents run as windows in the same `a` tmux session:
- Window 0: human shell
- Window 1: main claude session
- Window 2+: agent jobs (j-<name>)

Human and agents see the same tmux. Human can:
- `Ctrl-b <number>` to switch windows
- `Ctrl-b w` to list all windows
- View any agent's work in real time
