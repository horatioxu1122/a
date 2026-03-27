# Personal Agent Clone — Conversation Transcript & Plan
## 2026-03-26/27 — Sean Patten

---

## Transcript

### Starting point
Sean feeling frustrated about execution speed. Reviewed the `a` codebase — confirmed it's a genuinely rare system: self-compiling C polyglot, multi-device SSH fleet, append-only sync, job system (fork→agent→PR→email), performance regression enforcement, TUI/web UI. All in one context window. Runs on phones to servers.

### Core problem identified
**The gap between generating an idea and it actually running is too wide.** Ideas are abundant, execution is the bottleneck. Even `a` — built to close this gap — still requires too much activation energy to go from thought to running agent. This is worse on low-energy days (poor sleep, etc).

### Key ideas from session

**Long-running session memory:**
Agent sessions lose context when messages are deleted/compressed. Simple idea: extract high-value fragments from a session and reinject them. No existing mechanism for this. The gap is extraction + reinjection, not storage.

**Auto-spawner:**
Hub job runs periodically, reads notes/tasks, picks top priority unstarted one, marks as "ai-started", launches `a j <project> <prompt>`. Start with initiation only — no outcome management. Human reviews results via existing scream-driven workcycle.

**`a n` should trigger execution, not storage:**
Current flow: idea → `a n` → stored → forgotten → maybe reviewed → probably not executed.
Target flow: idea → `a n` → AI evaluates → if actionable, launches job → human reviews result. No human step between idea and attempt.

**Personal agent clone — the big idea:**
Long context (1M+ tokens) makes it actually possible now to create a personal agent that embodies your judgment, not just domain knowledge. This is not a chatbot that sounds like you — it's an execution amplifier that translates messy idea dumps into running jobs.

Almost nobody has done this because:
1. The capability (1M context) is months old
2. Most people don't have their thinking written down
3. The pipeline (collect→extract→consolidate→deploy) doesn't exist
4. People think clone = chatbot, not clone = execution amplifier

Sean has both the written corpus AND the execution infrastructure (`a j`). The missing piece is consolidation.

**Domain expertise vs judgment:**
Any agent can be given SQLite source or Linus's management style. But knowing that Sean would reject a 50-line fix when a 20-line rewrite exists, or that he prioritizes IBKR over tutorial polish because money buys time — that's his decision function. Only in his head and scattered across his writings.

The consolidation document isn't a knowledge base. **It's a judgment model.** That's why personal transcripts and notes are the training data, not textbooks.

**Simplified jobs beat autonomy:**
Given error rates that compound per turn (IDEAS.md #24), starting a simplified single-step job is better than full autonomy. Do one thing, human reviews, continue. Error chains stay short.

**Human-as-sole-router doesn't scale:**
Theoretically prefer flat structure (human→agent, no middleman). Reality: can't scale to 24/7 when human is active ~4-8hrs/day. Need some AI job management layer. The personal judgment model solves this — it IS the human in compressed form, routing on their behalf.

**Expert context stacking:**
Beyond personal judgment, agents benefit from expert context:
- SQLite source → code reliability patterns
- Linus Torvalds emails → software project management
- Gates/Kurzweil → future predictions
- Plus Sean's own context (IDEAS.md, notes, transcripts)

But the MISSING piece in previous experiments was deep understanding of Sean's way of thinking — not just expertise.

**AI successor agent:**
Not replacement but augmentation. For when human bandwidth is limited, an agent that continues work using the judgment model. Reduces dependency on Sean being awake and sharp.

### Actions taken
- Launched agent jobs on hsu, mac, ubuntuSSD — each reading tasks/notes and picking highest priority work
- Saved 7 notes capturing all ideas from session
- Created 12 tasks ranked by priority (P10000-P35000)

---

## Plan: Personal Agent System

### Phase 1: Collect corpus
**Goal:** Gather all of Sean's written output into one accessible location.

Sources:
- Google Drive writings (use existing rclone + `a gdrive`)
- Claude web chat exports (manual export or API)
- JSONL transcripts from all devices (already in `adata/backup/{device}/`)
- Existing notes (`adata/git/notes/`)
- IDEAS.md, AGENTS.md, README.md
- Any other written artifacts

Output: Raw corpus in `adata/books/` or similar, accessible to `a book` pipeline.

### Phase 2: Extract high-value content
**Goal:** Claude processes raw corpus, cuts noise, keeps insights/decisions/patterns.

Method:
- Use `a book transcribe`-like pipeline but for personal output
- First pass: MANUAL — Sean reviews and marks what's high-value to define "good"
- Second pass: Automate extraction based on patterns from first pass
- Key filter: Does this reflect judgment/decision-making, or is it just information?

Output: Extracted fragments, tagged by type (decision, insight, priority, pattern).

### Phase 3: Consolidate into judgment model
**Goal:** Book-length document that represents Sean's thinking and decision function.

Structure (tentative):
- Core values and priorities (from IDEAS.md + notes)
- Decision patterns (how Sean evaluates tradeoffs)
- Domain knowledge map (what Sean knows deeply vs surface level)
- Project context (current state, goals, blockers for each project)
- Communication patterns (how to interpret Sean's shorthand/typos/intent)
- Anti-patterns (what Sean rejects and why)

Output: Single document, loadable as system prompt or `--append-system-prompt-file`.

### Phase 4: Auto-spawner with judgment
**Goal:** Agent that receives raw idea dumps and translates them into executed jobs.

Architecture:
```
Sean (any energy level, any device)
  → dumps idea via `a n` or `a t add`
  → hub job fires periodically (every 30min)
  → agent loads judgment model as context
  → reads new unprocessed notes/tasks
  → translates to simplified single-step `a j` command
  → marks task as "ai-started"
  → launches job on available device
  → Sean reviews results when ready
  → continue or kill
```

Key constraints:
- One step per job (no error chains)
- Initiation only, no outcome management (human reviews)
- Agent asks for clarification via `a done` if insufficient context
- Deduplication across devices not critical to start

### Phase 5: Iterate
- Feed job results back into corpus
- Tighten judgment model based on what Sean accepts/rejects
- Gradually increase autonomy as trust builds
- Add expert context layers (SQLite, Linus, etc.)

---

## Priority
Phase 1 is mechanical and can start immediately. Phase 2 requires Sean's input to define "good." Phase 3 is the creative/hard part. Phase 4 is engineering on top of existing `a j` infrastructure.

**Start Phase 1 now. Everything else follows.**
