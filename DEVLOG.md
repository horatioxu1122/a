# a.c development log — 2026-04-02/03 session

## Thought-to-action: the scream

Current friction: have thought → open terminal → type `a j "do thing"` → wait.
On phone worse — keyboard slow, switching apps slow.

The conversation IS the interface. Talking to Claude with a.c system prompt
already works (a c). "add to md" happens. "push" happens. Missing piece:
persistence across sessions and dispatch without ceremony.

### Simplest version

`a <thought>` with no subcommand = universal entry point.
- If passive → save as note
- If actionable → dispatch as job
- No command to remember. Just talk to a.

### Web chat as thin view layer

The deleted dashboard was right idea at wrong time. Didn't have tmux
orchestration, multi-device fleet, or agent routing yet. Now we do.

The dashboard that failed tried to BE the system. The one that works is
a window INTO the system.

```
Web chat  → a j "build feature X"      (dispatch)
Feed      → see all agents running      (a ls / a jobs)
Card      → agent finished, show diff   (accept/reject from UI)
Push      → a push                      (from the UI)
```

Backend already built — every op is `a <cmd>`. Web layer is HTTP routes
calling commands. If web dies nothing breaks — agents run in tmux.

Build when scream comes from being away from terminal, not before.

### Messaging integration

OpenClaw (250K stars) solved messaging transport (WhatsApp/Telegram/Signal).

1. Ride OpenClaw: route messages to a's tmux sessions. ~20 lines glue.
2. Direct: Telegram bot API ~30 lines. Receive → `a j <msg>` → return.

Gates lesson: copy the insight not the product. OpenClaw insight: use the
interface people already have open.

### Competitor comparison

```
Claude Code:  terminal + tools + permissions    (Torvalds: composable)
OpenClaw:     messaging apps, local agent       (meet users where they message)
a.c:          terminal + tmux + multi-device    (fleet orchestration)
```

Gap vs OpenClaw: accessibility. 2 stars vs 250K. Terminal excludes 99%.
Gap vs Claude Code: CC is engine, a.c is fleet manager. Layers not competitors.

## LLM hallucination asymptote

Come and go with model generations — not converging to 0. Different error
distribution each time. ~1% persists at PhD level. More dangerous than 10%
at undergrad because higher trust, bigger consequences.

Cause candidates:
- A) Language combinatorial — plausible-sounding > true sequences
- B) Brain-like ceiling — human experts also ~1-3%
- C) Training artifact — RLHF, distribution shift

Workcycle "user runs debugs" isn't going away. Scream test catches the
asymptotic error rate.

## How to accelerate

Automate extraction of methods from greats. Manual process (6 months,
3 books → workcycle) could be:

1. LLM reads all books/interviews from top 100 in domain
2. Extract action patterns (what they DID, not quotes)
3. Find frequency across greats (convergent = fundamental)
4. Test via agent against current project
5. Keep what produces results

CFA applied to human methods. Hard: isolating action that caused success
from personality that happened to succeed. 100 things said, 3 matter.

## The three-body system

```
Gates:    make it shorter     (compression)    → a diff
Musk:     delete the part     (elimination)    → scream test
Torvalds: make it composable  (architecture)   → a <verb> dispatch
```

Sequence: architecture → elimination → compression. Compressing bad
architecture wastes effort.

## Pre-workcycle theories that failed

**Hierarchical folders + tests per level:** decomposition wrong first try,
tests lock in wrong boundaries.

**TDD / requirements as tests:** requirements wrong first try. Tests
encode wrong assumptions.

Both front-load decisions before information. Crap iteration does opposite.
These failures → "conservative to past best all time" method. Own theory
sample size = 1. Gates sample = Microsoft. Weaker filter.

## v7p / fredix insights applied to a.c

208 process creation calls in a.c audited. 70% are system interaction tax
(tmux, git, claude, ssh). 30% could be function calls (ls→readdir, grep
→memchr, file ops→pointer). Function call model applies to agent inner
loop (tool calls inside sessions), not system orchestration.

Internal file ops (freq cache, session state, config) could use v7 ramdisk.
Read hundreds of times per session. Currently 3 syscalls each. As ramdisk:
pointer dereference.

## Origin story notes

Path: gdrive (1)(2)(3).py → docker/gui/sqlite/tests (all deleted) →
terminal+tmux+single-file-C → workcycle from Gates/Musk/Torvalds →
56 commits/day for 7 months.

Key: every wrong thing tried was an indirection. Everything that survived
is direct. Terminal direct to OS. C direct to CPU. One file direct to
compiler. Scream test direct to inadequacy.
