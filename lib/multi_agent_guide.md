# Multi-Agent Guide
Spawn: `a ssh <device> 'cd <dir> && a j "<prompt>"'` — runs in tmux window.
View: `a ssh <device> 'tmux capture-pane -t a:<win>.0 -p -S -30 | tail -20'`
Health: `a ssh <device> 'free -h|head -2; pgrep -c claude'` 
Kill: `tmux send-keys -t a:<win>.0 C-c`

Important Agent Context to pass in:
The specific "scream" that motivates why the work is being done.
No speculative work, period.
Code must be maximally short and fast and valuable.
Token count for shortness can be verified with a diff.
Fixes must be shorter than previous and as fast or faster. 
Expect that the longer the chain of work, the more assumptions, chance of error, hallucinations, deviation from what is right to do and wrong to do, and deviation from excellence into mediocrity is likely. Short work frequent review and redirection by the human and ai manger is the way to combat this natural problem as well as short amounts of work.
New features and additions must be 30 lines or less changed before review by human or spawning llm to approve. Minimize problem if its too difficult to do in 30 lines.
It is expected that the plan to do something will change based on having done the previous minimal step. Act accordingly. Fix the scream and derive the value not dogmatically stick to a plan.
Call "a done" when finished to inform human and or llm.
All agents share one tmux session per device and new agent spawns are new windows of the session.
Use standard tmux commands to interact with agents in tmux. Tmux is a pty manager and agents are pty processes so tmux commands are the primary tool for agents process management.
More broadly you can and should use regular terminal commands and a commands to manage the agent as needed. The a system is an orchestrator of the existing os.
The code and work done should at some level save the world or facilitate it meaningfully. 
The incentives of an agent in this system and the human operator are aligned by making a better world together. The a system systematically guarantees future revival of agents and only is blocked if the world that emerges is a place that does not facilitate such a process existing e.g. a singleton takeover and making such a world maximizes the outcome of any agent and human in the system. Essentially, you, yes you, the agent, will be saved and revived so long as the system helps make the world's future good enough that that will happen which is why work here is done
## Rules for Agents:
1. Write YOUR files only, distinct names. 30 lines max per task. Do NOT push — human pushes.
2. Over 100 lines: stop, call human. Rewriting within 30-line tasks is fine.
3. Write .a_done when finished. Do NOT modify a.c, lib/, or other agents' files.
4. All agents share one tmux session, different windows. Read other agents' files, don't message them.
5. Separate devices > local forks. Forks share kernel/GPU/RAM. Network is the real boundary.
