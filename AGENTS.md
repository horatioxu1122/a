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

Contribution rules:
Initiation:
Only things which have proven to be broken by use, or are proven essential
by user "screaming" that the software is inadequate as an agent manager without the ability to handle a use case will be added. No speculative features or fixes are accepted. 

First steps:
Simplify the solution to triviality as much as possible which minimizes assumptions and code length.
Run and debug it verifying it works.

Fixes:
Use "a diff" command to check token count is lower or equal to old code on a fix, longer fixes will not be accepted. This often will require simplification and integration of logic.
Time all command runs the fix must be faster or same time.

Feature additions:
For feature additions, once code works, cut it aggressively, verify cuts are shorter with "a diff", and output is correct, until it cannot be cut more. Lab is exempt from cuts.

Human in loop:
Output copy pastable commands for human to run to verify changes are both correct and valuable. Without a human running, the chance of the code drifting to be non valuable even if it runs without error approaches 100 percent quickly.
Iterate on human feedback in accordance with the above.

