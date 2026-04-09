## Principles
1. As a general principle, tools must have an interface that works well for both LLM agents and humans. This is crucial for alignment and collaborative human-AI work because it gives a shared function set and information flow to the user and LLM.
2. Speed for the user is essential, as is minimizing keystrokes and input actions to accomplish anything. Especially mobile users need chars minimized at all cost.
3. There should be no polling, only event-driven processes.
4. OS program dependencies and software dependencies must all be handled by the script, not ad hoc install per machine. If the script fails to work on a new machine because of a missing dependency, it's the script's fault for not auto-installing it properly, not the user's fault.
5. Exceeding the time limit should kill the program, and lack of completion is an error to fix by faster software.
6. Scripts should expect to die and save persistent data to adata given that expectation.

## Organization
7. All persistent data goes in adata, period. adata/git will hold user personal git-synced data.
8. All logic goes to /lib. /lib files should be as independent as can be from one another, but some shared logic is inevitable.
9. a.c handles build, dep install, and dispatch to /lib.

## Rules
10. Software is killed at 1 sec by default, and limits are auto-tightened continuously by best time of past through a perf. Network-dependent operations may be 5 sec at max.
11. If software must operate indefinitely, like waiting for user input or running a server, it must reach a no-op, no-mem-change state before disarming the timer.
12. No command or operation should ever be exempt from time killing except the no-op state and necessary dep downloads. Download the highest-priority deps first. The number of deps and time to install must be minimized.
13. First-time installation, compiling, and installing dependencies are also program operations under time limits, with a dep download time exception.
14. Use C unless unsuitable.
15. All commands and subcommands must be accessible in the a i tui.
16. Commands should follow the format: 3 chars or less, occasionally 4, never more; same name as file unless file handles many cmds.
17. A cmd with no parameter should show the menu of commands, how to type them, and the obvious most common information the user wants.
18. All sub cmds must be one char to call. cmd <text> should do the obvious thing when given text the user wants most often.
19. Show all subcommands exactly as you would type them to call them, and explain what they do in 4 words or less.
