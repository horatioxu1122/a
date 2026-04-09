As a general principle, tools must have an interface that works well for both llm agents and humans both. This is crucial for alignemnent and collaborative human ai work because it gives a shared function set and information flow to user and llm
Speed for the user is essential as is minimizing keystrokes and input actions to acomplish anything. Esp mobile users need char minimized at all cost.
Exceeding time limit should kill the program and lack of completion is an error to fix by faster software.

Important organizational principles:
All persistent data goes in adata period. adata /git will hold user personal git synced data. 
All logic goes to /lib. /lib files should be indepndent as can be from one another but some shared logic is inevitable.
a.c handles build, dep install, dispatch to /lib

Specifically:
all commands and subcommands must be accessible in the a i tui.
There should be no polling, only event driven processes.
If software is dependent on any process like network operations they must still complete in 5 sec or be killed ideally 1 sec.
Software is killed at 1 sec by default and limits auto tightened continously by best time of past through a perf.
If software must operate indefinitely, like waiting for user input or running a server, the killing must still occur but disarm when the waiting op is done. However waiting must be a no op, state changes also get killed on time. 
no command should ever be excempt from time killing.
time killing is only enabled if the user has set it on.
scripts should expect to die and save persistent data given that expectation to adata
Favor c if possible, but python or other languages should be used for library avaliability.
Commands should follow the format: 3 char or less occsionally 4 never more, same name as file unless file handles many cmd, 
cmd no parameter should show the menu of commands and how to type them and the obvious most common information user wants,
all sub cmd must be one char to call. cmd <text> should do the obvious thing when given text user wants most often.
Show all subcommands as you would type in to call them exactly.
