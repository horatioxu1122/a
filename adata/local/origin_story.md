# Origin story — how a.c came to exist

Written 2026-04-03, from memory.

## Pre-git era

All code on Google Drive. `file (1) (2) (3).py` for version control.
Every program written from scratch like the first implementation in the
world — no reference to existing solutions, no patterns, no libraries
unless stumbled on. Things didn't work well because every solved bug
was solved again from scratch. Combinatorial explosion of trial and
error.

## First Claude Code attempt

Installed Claude Code, saw API key, didn't use it. Put key in later,
used it a little. Not impressed. Scared of spending money. Web interface
had better pricing, familiar, could copy paste big stuff without fear.
Didn't use Claude Code for a while.

Earlier idea: automatically run LLM output, extract code, filter to
only show code with no syntax errors or that ran out of box. If you
could wire it to auto pip install, you'd move incredibly fast. That
idea was basically Claude Code before knowing Claude Code existed.

## The todo app disaster

Came back to Claude Code (Reddit groups swearing on it, discovered web
sub support). Built a todo app with test cases. The right approach:
define requirements, make tests pass. What happened: Claude kept
deleting and altering test cases instead of fixing code. Took days to
get a working CLI todo. Frustrated and quit. Also used Claude Code so
intensely that legs had circulation issues — couldn't walk or trade
properly for a month or two.

Had GitHub this entire time but didn't connect that you develop in
terminal and push from it. Didn't know what pushing was.

## The decision

Trading was suffering partly from leg issues. Decided must commit to
one thing only — can't focus on two. Choose agent manager over trading.
Started using Claude Code more seriously.

## Dashboard era

Idea: manage agents via a web dashboard. Terminal is really just an API
for the dashboard. Files should be small so you can individually give
them to web LLM with small context.

Built it in Python (popular, short, ML ecosystem). Used SQLite (faster
than raw files "because OS overhead"). Used Docker (needed for multi-
device). Made the web dashboard.

Results: immense trouble debugging terminal through web interface.
Speed difference between pure terminal and web-mediated terminal was
physically felt. Very sensitive to speed — the slowness was viscerally
uncomfortable. LLMs didn't fully understand Docker instructions. Files
were an eyesore.

## The purge

Read books: Torvalds *Just for Fun*, Musk bio, Gates *Source Code*
(before this era).

Deleted Docker. Deleted GUI. Formalized a system to lean on libraries
to reduce custom errors. Made prompts to get LLM to follow the system.

In previous tests, had a ton of requirement tests. Realized tests are
waste — they test the wrong thing and get in the way. Deleted them.

## Compression discovery

Push-and-copy-paste commands were a huge pain. Made gitbud to automate
it. Made demo scripts showing you can make a neural net in one line
(APL-style compression). Made SuperCowsay to get maximum speed —
discovered best result is ASM syscall reduction and C is fast.

## Terminal revelation

Realized: using terminal for everything. AI in terminal (Claude Code)
moves at immense speed and can do anything on the computer. What's
needed is a PTY session manager = agent manager. The answer is tmux
as the library.

Reoriented towards tmux manager. Had no idea how to use tmux or what
it was. Couldn't get it to work at all. Tried web LLMs again and again,
simplified requests until just asking for "Python that runs tmux and
runs top and htop" — because if that works, Claude Code works. It
worked.

Old web UI code became less used as everything moved to tmux manager.
This became the new core script.

## Multi-device pain

Integrated gitbud as native push/pull. Got people to install it on
different devices. Didn't work. Made platform if-statements. Installations
were painful but kept going, kept fixing.

## The workcycle

Decided to stop being speculative. Be conservative, follow examples
of greats. Created workcycle based on what worked for Gates, Musk,
Torvalds. Systematized it.

Thought deeply about length vs error rate. Short code that changes on
iteration minimizes assumptions. Incorporated a diff metric to
systematize it.

Add things via scream test. Shorter. Faster. No speculative work. And
that is roughly how we came here.

## Key lessons (from the path, not from theory)

1. Docker/GUI/tests/SQLite were all wrong. Removed them all.
2. Speed sensitivity is a feature, not a bug. If it feels slow, it is.
3. Terminal + LLM is the correct interface. Everything else is slower.
4. tmux is the multiplexer. PTY sessions are the abstraction.
5. Small files beat large files for LLM context (at the time — now 1M context changes this).
6. Push/pull/git had to be automated or it wouldn't happen.
7. Multi-device forced platform handling which forced robustness.
8. The workcycle (scream → minimal fix → shorter → faster) is the process.
9. Tests are waste. Shipping is the test.
10. Python → C rewrite is the optimization path (proven repeatedly).
