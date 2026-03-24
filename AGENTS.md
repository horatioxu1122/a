Project context steps:
Make sure the version is latest main.
Read output of "a cat 3" to see codebase.
Read a.c and use the index to jump as needed to see locations by searching.

General information:
Write code as short as possible, readable, fast as possible, using direct library calls over custom logic.  
Logic goes in flat files in lib folder. 
Experiments in lab folder.
All persistent data lives in adata folder.
Don't push without approval. 
The optimal program is maximally short fast and valuable. Edits should converge towards this.
Issues with environment, ex dependency is not working, should be fixed by modifying a.c to systemically fix issue in code for all users not one off single device fixes.
You may be working with other agents on the same codebase, if files change mid session be aware this is normal. If it stops your work, stop and use “a done <message>” for the user to intervene.

Contribution rules:
Initiation:
Only things which have proven to be broken by use, or are proven essential
by user "screaming" that the software is inadequate as an agent manager without the ability to handle a use case will be added. No speculative features or fixes are accepted. 

First steps:
Simplify the solution to triviality as much as possible which minimizes assumptions and code length.
Run and debug it verifying it works.

Fixes:
Token counts and timing are auto-provided on every command run via stderr for immediate local signal.
Use "a diff" to compare against main before pushing — token count must be lower or equal on a fix, longer fixes will not be accepted. This often will require simplification and integration of logic.
Time all command runs the fix must be faster or same time.

Feature additions:
For feature additions, once code works, cut it aggressively, verify cuts are shorter with "a diff", and output is correct, until it cannot be cut more. This should be done by cuts that halve the new tokens or more each time. If the user says "crunch" they want it halved with same functionality until it breaks. Lab is exempt from cuts.

Build workflow:
Use "sh a.c" for fast iteration (instant O0 build, checkers run async in background).
Use "sh a.c check" before presenting work to the user (runs all checkers foreground, exits 0 on pass, O3 in background).
Never present code to the user that hasn't passed "sh a.c check".

Human in loop:
Run debug code before declaring it done.
Output copy pastable commands for human to run to verify changes are both correct and valuable. Without a human running, the chance of the code drifting to be non valuable even if it runs without error approaches 100 percent quickly.
Output a diff token change numbers and time of command info.
If main has advanced in the time since you started working, merge in the main changes to your code before delaring a done. 
WHen human approves, use "a push" over git directly, it will handle parallel work better. Use "a done" to get human if there is a push issue.


