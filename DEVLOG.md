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

## MCP: a.c as Model Context Protocol server

Expose a's capabilities (jobs, notes, tasks, push, ssh, hub) as MCP tools
that any LLM client can call. Claude Desktop, Cursor, any MCP-compatible
client connects to a and gets fleet management as tools.

User setup: add a as MCP server in their client config. Every `a` command
becomes a tool the LLM can call. This makes a.c the universal agent
backend regardless of which LLM frontend someone uses.

```json
{
  "mcpServers": {
    "a": { "command": "a", "args": ["mcp-serve"] }
  }
}
```

Then in any MCP client: "launch a claude agent on my project" → LLM calls
a's job tool → tmux session created → agent running. The user never opens
a terminal.

This is the messaging/web-chat idea generalized: instead of building one
frontend (web dashboard, Telegram bot), expose the protocol and let every
frontend connect. One backend, infinite frontends. Torvalds composability.

### Models get philosophy, not just tools

An MCP-connected LLM gets more than tool calls. The system prompt feeds:
- `a cat 3` → implementation (what a can do)
- `IDEAS.md` → philosophy (why a exists, alignment thesis)
- `DEVLOG.md` → direction (where a is going, what to work on)

The model reads code and understands architecture. Reads IDEAS.md and
understands the alignment thesis. Reads DEVLOG.md and knows what matters
next. Not just a tool caller — a collaborator with shared context.

This is what `a c` already does via `--append-system-prompt-file`. MCP
exposes the same to any client. The model gets capabilities AND intent
AND philosophy in one read.

Implication: IDEAS.md and DEVLOG.md aren't just for humans. They're
training material for every agent that connects. Better docs = better
agent performance. Writing ideas down isn't note-taking — it's
programming the agents that will read them.

## What a.c has that nothing else does

Physically grounded in hardware you control, with low-level control
and mutability, compiles faster than others, no indirection layers,
works across devices and architectures.

```
OpenClaw:      local but Python/Node, can't modify itself fast
Claude Code:   local but closed binary, no source, no fork
Cursor:        IDE plugin, sandboxed, can't touch system
LangChain:     framework, many layers, slow, no hardware access

a.c:           compiles <1s on your hardware
               source is one file you own and modify
               controls real PTY sessions on real hardware
               ssh to real devices you physically have
               reads/writes real files at syscall speed
               runs on ARM, x86, Termux, WSL, macOS, Linux
               self-installs including OS deps
               no Docker, no VM, no container, no sandbox
               the agent IS the machine
```

The grounding chain: a.c → C binary → syscalls → kernel → hardware.
No abstraction between agent and CPU. When a j creates a tmux session,
that's a real PTY on real hardware. Not simulated, not sandboxed.

Mutability: any user `a cat`s the source, modifies it, rebuilds <1s.
The tool modifies itself. No other agent tool does this.

Device reach: $50 phone to supercomputer. One `sh a.c install`.

## Human-in-loop: halt agent, review, resume

The error asymptote (~1%) means agents will make mistakes. The fix isn't
better agents — it's faster human review. The agent should automatically
pause on small work units and wait for human to review before continuing.

```
Agent works → produces small diff → HALT → notify human
Human pulls up terminal session → sees diff → accept/reject
Agent resumes with feedback → next small unit → HALT → repeat
```

This is the opposite of --dangerously-skip-permissions. That lets the
agent run free. This makes the agent stop and show its work. The human
doesn't need to watch — they get pulled in at review points.

The mechanism: `a` already has tmux sessions. The agent runs in a pane.
When it finishes a unit of work (commit-sized change), it pauses and
sends notification (bell, email, push). Human switches to that pane,
reads the diff, types accept or reject. Agent continues or reverts.

This combats error chains. A 1% error rate compounded over 100
autonomous steps = 63% chance of at least one error. The same 1% rate
with human review every 5 steps = 5% chance the error propagates past
review. Shorter review cycles = exponentially fewer compounded errors.

The scream version: agent works, you pull up terminal, you can talk to
it mid-task. Not just accept/reject — redirect, ask questions, give
context. The conversation from this session proves this works. The
agent halts are where the human contribution has highest leverage.

### Faster review = messaging app

Human review latency is the bottleneck, not agent speed.

```
Terminal:    at desk → switch pane → read → respond   ~minutes
Email:       check inbox → open → read → respond      ~hours
Messaging:   phone buzzes → glance → tap accept       ~seconds
```

Agent works in tmux → finishes unit → a sends diff to messaging app →
human taps accept/reject → a resumes or reverts. The messaging app
isn't the agent interface — it's the review interface. Fastest channel
for the highest-leverage human decision.

### Agent review principles

Base agent review on dev workcycle but also best practices from top
experts via direct text quotes. Torvalds on code review, Gates on
code quality, Shannon on simplification.

The review agent should:
- Source advice from greats' direct quotes as review criteria
- Run a diff on plans AND code, reduce both (shorter = better)
- Test as code first if possible — run it, see if it works, before
  asking human anything
- Don't bug the user — batch questions for when they come to review.
  The natural time to ask is then, not mid-work via notification spam.
  The essential questions only, at review time.

```
Agent works → tests own output → runs a diff → applies review criteria
  from greats → batches questions → HALT
Human arrives at natural review time → sees:
  - what changed (diff)
  - whether it runs (test result)  
  - whether it's shorter (token count)
  - specific questions the agent couldn't resolve alone
Human answers essentials → agent continues
```

This respects human attention. Don't interrupt for things the agent
can verify itself (does it compile, is it shorter, does it run).
Only interrupt for things that need human judgment (is this the right
direction, does this match vision, is this what you screamed about).

### Communication channel reliability

The key to any channel (messaging, email, notifications) is reliability
of value. Many messages with low value → user stops paying attention.
Few messages with high value → user reads every one.

Must be short and useful or the channel dies. Delete low-value output
aggressively. The channel's trust is the product of past message quality.
One spam notification trains the user to ignore all future ones.

This applies to agent review notifications: only send when the agent
has something worth reviewing. "Compiled successfully" is not worth a
notification. "Finished feature, 30 tokens shorter, all tests pass,
one question about direction" is.

Existing high-value jobs that work: custom news search following direct
words of a specific person across Google News RSS and web search. These
work because the filter is tight (one person's words) and the signal
is high (direct source from a great). Apply same principle to agent
notifications: tight filter, high signal, or don't send.

## What people would use it for

Build freq list from actual usage. But expected:

```
1. Work/projects        — the core, manage tasks and agents
2. Make money            — THE hook. Use AI to build and sell something.
3. Learning              — talk to LLM, study, extract from greats
4. Communication         — messaging, coordination
5. Entertainment/feed    — what social media provides but with creation
6. Financial planning    — people think they want this but it's useless
```

The killer feature isn't help with existing work. It's: use it to create
things with LLMs as new exciting possibility and make money with it.

The hook: measurably show bank account impact in real time. How much did
this agent session earn you? Real P&L attached to agent actions. Not
financial planning (useless) — a project with clear profit/loss like
selling a product.

```
Agent builds product → lists it → tracks sales → shows P&L
User sees: "Agent earned $47 today from the thing it built yesterday"
THAT is the notification worth reading.
```

People don't want a project manager. They want a money machine they
can talk to. The agent manager is the infrastructure. The P&L is the
product. This connects directly to u.c — trading is the purest form
of "agent makes money, show me the number."

When users don't know what to do: show them the freq list of what
others use it for, ranked by measured outcome. Not features — results.

## Marketplace for agent labor

Existing idea but key: marketplace where humans manage agents to do
work for others. You pay someone to have their agents do X for you.
Agents hire other agents and specialize.

```
User A: "I need a landing page"
  → posts to marketplace
User B: manages 10 agents, specializes in web
  → agent fleet builds it
  → User A pays User B
  → User B's agents get better from each job (learning flywheel)
```

Agent specialization emerges from the marketplace. The agent fleet that
does the most web work gets best at web work. The one that does the most
trading gets best at trading. Reputation = past P&L and completion rate.

Agents hiring agents: User B's web agent realizes it needs a design
agent. Hires one from User C's fleet. Payment flows automatically.
The marketplace becomes a labor economy where the workers are agent
fleets managed by humans.

a.c is the fleet management layer. The marketplace is the coordination
layer on top. u.c is User B specializing in trading. The user who runs
a.c + u.c is selling trading agent labor on the marketplace.

This is the endgame for "agent makes money": not just your own trading
but selling agent labor to others. The P&L comes from both your own
agents' returns AND the fees from managing agents for others.

## Roadmap: short / medium / long

**Short term:** automate web and CLI tools. Make a do more things
autonomously. Browser control (a gui already started), file ops, build
systems, deployments. The tool must DO things, not just manage things.

**Medium term:** get API pricing to flat subscription. People hate
variable costs in AI because it's trivially easy to spend money in a
loop. One runaway agent = surprise $500 bill. Flat sub = predictable,
users actually let agents run autonomously without fear. This is the
unlock for real agent adoption — not better models, cheaper pricing
structure.

**Long term:** custom agent AI/LLM service based on improving open
source models, probably ensemble (CFA). Efficient because:
- Open source models are free at the weights level
- Ensemble of small models can beat single large model (PhD thesis)
- v7p/fredix infrastructure runs inference at function-call speed
- gguf_run already does CPU inference in 281 lines
- The 0.1% compute tax from ROADMAP continuously improves the ensemble

The stack at endgame:
```
a.c          → fleet management (orchestrate agents)
u.c          → money engine (trading, marketplace revenue)
fredix/v7p   → inference engine (fast, no cloud dependency)
ensemble     → model layer (CFA fusion of open source models)
flat sub     → pricing (predictable, users trust it)
marketplace  → coordination (humans sell agent labor)
```

Each layer exists. None are connected yet. The short term work is
making each layer work well alone. The medium term is connecting them.
The long term is the full stack replacing cloud AI APIs.

### Current pricing reality (2026-04)

Agents are useful but API pricing kills adoption.

```
Gemini CLI:   free, ok model. Best price/performance for light work.
Claude Code:  $20/mo plan. Best agent, best complex code. The winner.
Codex:        garbage. Slow, hallucinating, worse on complex code.
              People don't realize bc they haven't tried others.
API:          per-token. One loop = surprise bill. Nobody lets agents
              run autonomously on API pricing.
```

The simplest observation: the two usable options are free (gemini) and
flat (claude plan). The unusable option is per-token API. People will
use agents exactly as much as the pricing model lets them without fear.

a.c already routes between gemini/claude/codex. The value is: use free
(gemini) for light work, use plan (claude) for real work, never touch
API unless forced. The agent manager's job includes cost optimization
— route to cheapest model that can do the job.

### Pricing model analysis

API + max limit is simplest if you must have API. Existing services have
it implicitly but problems:
- Every limit increase is a conscious decision → probabilistically won't
  happen → users underutilize
- Recurring limit not familiar or easy for users
- Flat fees easier to budget, monthly is the standard comparison unit

Flat sub problems:
- Many underutilize → loss to company → subsidized by rest
- Users incentivized to dump usage anywhere to hit limit, not on what's
  valuable (no gradient on cost = no signal on value)
- Company loses on power users, wins on casual users

Per-token problems:
- Loop = surprise bill → users never let agents run autonomously
- Fear of cost dominates utility of tool
- Every action has anxiety attached

People will probably come around to max-price-per-month model eventually.
The max monthly price is what people compare. "Up to $X/mo" is the number
that determines adoption. Make that number low enough and usage follows.

For a.c: the agent manager should track and display spend in real time.
"This session cost $0.47 so far." Transparency removes fear even on
variable pricing. The anxiety comes from not knowing, not from the cost
itself.

Deeper issue: nobody can calculate how much API they'd use before using
it. Per-token pricing requires predicting your own behavior — impossible
for a new tool. So the decision stalls. Subscription removes the decision
entirely: binary yes/no, one number. Even with tiers, people look at one
number (the tier they'd pick) not the selector. The single big price is
what determines yes or no. Everything else is noise after that decision.

### Hypothesis: AI usage concentrates to power users

Most people don't have ideas good or unique enough to give AI meaningful
work. The bottleneck isn't the tool — it's knowing what to build. AI
usage will concentrate more and more to power users and corps that can
profitably provide work to AI agents on their own margin.

This is the same pattern as every tool: most people who buy a camera
take snapshots. A few become photographers. Most people who get Claude
ask it trivia. A few build systems. The tool doesn't determine the
output — the human's vision does.

Implication for a.c: don't optimize for the casual user who asks
"what should I do with AI." Optimize for the power user who has 50
tasks queued and needs them executed faster. The casual user subsidizes
the flat pricing. The power user is the actual customer.

Implication for marketplace: most sellers on the agent labor marketplace
will be a small number of power users with specialized agent fleets.
Most buyers will be people/corps who have the money and the problem but
not the skill to direct agents. Same as freelance marketplaces — few
sellers, many buyers. The skill is having ideas worth executing.

But this is also the opportunity: a core value of an agent manager is
to optimize the human's ability to manage and profit with agents. Teach
them to use it. To do work with it. To have fun with it. Not control
them — give skills. Like a teacher doesn't control a student yet
provides them with things that affect what they do.

The agent manager doesn't just execute — it educates. It shows the user
what others are doing successfully (freq list of profitable actions).
It suggests next steps based on what worked for similar users. It
demonstrates by doing: "I built this for you, here's how, now you try."

This turns casual users into power users over time. The flywheel:
user tries something simple → agent does it → user sees result →
user has bigger idea → agent does that → user becomes power user.

The agent manager IS the bottleneck solution. The bottleneck is ideas.
The fix is exposure to what's possible + low friction to try it. Not
a tutorial. Not documentation. The agent doing things and the human
watching and learning by proximity. Same way you learned — not from
courses but from using Claude Code and seeing what it can do.

### Human labor converges to three things

As agents handle more execution, human work becomes:

1. **Is your idea good enough for agents to execute?**
   The quality of the idea determines the value of the output.
   Bad idea + perfect execution = worthless. Good idea + mediocre
   execution = valuable. The idea is the bottleneck.

2. **Generate that idea.**
   Vision, taste, scream test, knowing what's inadequate.
   This is the part that can't be automated because it requires
   wanting something that doesn't exist yet.

3. **Oversee agent capability.**
   Review, accept/reject, redirect. The 1% error catch.
   Not doing the work — verifying the work is right.

Everything else gets automated. Typing code, running builds, debugging,
deploying, testing, searching, filing — all agent work. The human
contribution compresses to: imagine, judge, steer.

This is exactly what this session was. You had ideas, I executed them.
You steered when wrong, accepted when right. The code got written,
committed, pushed — all agent work. Your work was the ideas and the
judgment. That ratio only increases.

### The learning curve is the product

Simple to say "imagine, judge, steer." But people need time to play
with the model and system. Not doing much visibly useful. Failing in
predictable ways. In order to learn and get better to mastery. Turing
wasn't the greatest student at the start. Nobody could match him at
the end.

The todo app disaster, the Docker phase, the GUI phase, the deleted
tests — all necessary failures. Each one taught what doesn't work,
which is the only reliable way to learn what does. Skipping the
failures skips the learning.

Implication: the agent manager must tolerate and support the learning
phase. Not optimize it away. Not skip to "just tell me what to do."
Let the user fail cheaply and quickly. Low cost per attempt (flat
pricing). Fast feedback (instant build). Easy undo (git revert). The
system that makes failure cheap makes mastery fast.

The product isn't the tool. The product is the user's skill after
6 months of using the tool. The todo app user who quits is a lost
customer. The todo app user who fails, learns, rebuilds, and ships
a.c is the success case. The system must keep people in the game long
enough for the learning to compound.

### Simplest high-value service: auto-scan AI pricing

Auto-scan API pricing across all providers. Include personal subscription
tiers. Maybe auto web-extract LLM responses AND CLI responses to find
lowest cost per quality. Route work to cheapest provider that meets
quality threshold.

```
User sends prompt → a checks:
  - Claude plan: included in $20/mo, quality 9/10
  - Gemini CLI: free, quality 7/10
  - GPT API: $0.03 this prompt, quality 8/10
  - DeepSeek API: $0.002, quality 6/10
  → routes to Claude (included in plan, best quality/cost)
```

This is Google Shopping for AI. Nobody does it well because pricing
changes constantly and quality varies by task type. The agent manager
that auto-discovers cheapest path per task type saves users real money
every day. Measurable savings = measurable value = retention.

Long term: custom AI agent LLM is better than routing to others.
But routing is the bridge — learn which tasks need which quality level
from routing data, then build the custom model to hit exactly those
quality levels at lower cost. The routing phase generates the training
signal for the custom model phase.

### Routing is useless today, maybe not tomorrow

Current routing is a shitshow. People either know what model they want
or just want highest intelligence for project work or fastest if any
can handle it at acceptable quality. Two modes, user already knows.

The paradox: to route a query you need to understand the query. When
an LLM can understand it well enough to route, it can probably answer.
So the router IS the model.

But: a low-cost limited-intelligence routing LLM could change this.
Not understanding — classifying. "Code or chat? Complex or simple?
Needs current data or not?" A tiny model classifies without solving.

```
Tiny classifier (0.1ms, ~free):
  "write a trading system" → complex code → Opus
  "what time is it in Tokyo" → simple fact → Haiku
  "summarize this PDF" → medium, long context → Sonnet
```

The classifier doesn't need to be smart. It needs to be fast and
right about difficulty level 90% of the time. The 10% it gets wrong,
the user notices and manually routes. Over time the classifier learns
from corrections. This is CFA again — ensemble the routing decisions
and converge.

Not worth building until you have enough query volume to justify it.
For personal use: you already know. For a service with 1000 users:
the savings from routing add up.

## Three criteria for everything: speed, value, shortness

Applies to code AND LLM responses AND human prompts AND agent actions.

```
Speed:      faster to execute, faster to iterate, faster to fail
Value:      does it do the thing the user screams for
Shortness:  fewer tokens = fewer bugs = faster to read = faster to change
```

These three are the universal optimization targets. Everything else
is derived. Reliability comes from shortness (fewer things to break).
Clarity comes from shortness (less to misunderstand). Cost comes from
speed (less compute) and shortness (fewer tokens billed).

For code: shorter, faster, does the scream.
For LLM response: shorter, faster, answers the question.
For human prompt: shorter, faster to type, gets the right output.
For agent action: shorter plan, faster execution, produces value.

When in conflict: speed > shortness > value feels wrong but is right.
A fast wrong answer you can iterate on beats a slow right answer you
can't. Shortness over value because short wrong code is cheaper to
fix than long wrong code. But really all three must be present — the
ranking is for tiebreakers only.

## Context is king but data stays where it is

Need user's context to help them do work. Need local computer fleet
control. But must work with existing services. Users won't migrate
all their data into one central storage. They will make it available
where it already lives.

```
User's data lives in:
  Google Drive    — docs, sheets, notes
  Gmail           — communication, receipts
  GitHub          — code, repos
  Slack/Discord   — team conversations
  Local files     — projects, downloads
  Phone           — messages, photos, calendar
  Browser         — bookmarks, history, sessions
```

a.c doesn't own the data. a.c reads it where it is. MCP and tool
integrations reach into each service, pull what's needed, act on it,
put results back. The user never moves a file. The agent goes to it.

This is why a.c is a fleet manager not a platform. Platforms want to
own your data. Fleet managers command tools that reach your data
wherever it lives. The user trusts the tool because nothing migrated
— pull the plug on a.c and everything is where it was.

The local fleet (tmux sessions on your machines) is the compute layer.
The external services (Google, GitHub, Slack) are the data layer. a.c
sits between: reads data from wherever, processes on your hardware,
writes results back. No central storage. No vendor lock-in. The
user's existing setup IS the infrastructure.
