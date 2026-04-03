# Android Terminal & Claude Code — Research Notes

2026-04-02. Everything discovered in one session.

## The Problem

Termux keeps dying on Android. Background process killed by OS. Claude Code runs in Termux. When Termux dies, work dies.

## Immediate Fix Applied

Three ADB commands that may solve it without building anything:

```bash
# Disable phantom process killer (Android 12+)
adb shell "device_config put activity_manager max_phantom_processes 2147483647"

# Allow Termux background execution
adb shell "cmd appops set com.termux RUN_IN_BACKGROUND allow"

# Whitelist from doze/battery optimization
adb shell "dumpsys deviceidle whitelist +com.termux"
```

Also enabled wireless ADB: `adb tcpip 5555 && adb connect 192.168.1.154:5555`

**Test this first.** If Termux stops dying, everything below is unnecessary.

## Phone Details

- Pixel 10 Pro, Android 17 beta
- No global notification vibration pattern setting exists on stock Android (Samsung-only feature)
- Pixel Sounds v3.3 has vibration patterns built but server-side flagged off by Google

## What Claude Code Actually Is

- **189MB self-contained Mach-O arm64 binary** (on Mac)
- **Runtime: Bun** (not Node.js). JavaScriptCore engine, not V8
- 163K lines of TypeScript, bundled into Bun binary
- The core logic: HTTP POST to `api.anthropic.com/v1/messages` with tool definitions, execute tools, loop
- nanocode proved this core is ~250 lines. The other 162,750 lines are UI, permissions, telemetry, IDE integration

## Auth Constraint

- Subscription pricing (Max plan, 7-15x cheaper than API) only works through Claude Code's OAuth
- Token prefix: `sk-ant-oat01-*`
- **Locked to Claude Code binary** — Anthropic rejects these tokens from non-official clients
- Client fingerprinting active. Accounts banned for third-party use
- **You must run the real Claude Code binary to get subscription pricing**

## Prompt Caching (if using API directly)

| | Write cost | Read cost | TTL |
|---|---|---|---|
| Standard | 1.25x base | 0.1x base | 5 min |
| Extended | 2x base | 0.1x base | 1 hour |

100K token codebase context: $0.50/call without cache, $0.05/call with cache hit. But subscription is still cheaper.

## Architecture Options Evaluated

### Option 1: Fix Termux (minutes, already done)

ADB commands above. Termux + Claude Code continues to work. Test if it holds.

### Option 2: Custom APK with Bun + Claude Code (weekend)

Ship Bun arm64 binary (~90MB) as native lib in APK. Bundle Claude Code JS. Own foreground service that Android cannot kill. PTY via JNI forkpty. No Termux dependency.

Bun already works on Android arm64:
- [bun-on-termux](https://github.com/tribixbite/bun-on-termux) — glibc-runner approach
- [bun-termux](https://github.com/Happ1ness-dev/bun-termux) — lightweight wrapper, no proot

### Option 3: v7p on Android (the long-term vision)

v7p.c from fredix project: Unix V7 as a portable process. 768KB binary containing V7 kernel + TCC + MicroPython + LLM client. Everything runs as function calls, no fork/exec.

**Strengths:**
- Sidesteps W^X entirely (nothing exec'd, everything is function calls / JIT to memory)
- 4ms compile time
- TCC compiles CPython (632K lines, 0.34s), bash, tmux from source
- `mmap(PROT_EXEC)` on anonymous memory is allowed on Android (JIT engines use this)
- TCC 0.9.28rc (Dec 2025) confirmed: bash.static, python.static work

**What TCC can compile (all C):**

| Tool | Lines | TCC status | In v7p |
|---|---|---|---|
| CPython 3.14 | 632K | Proven, 0.34s | MicroPython already in |
| bash | ~100K | Proven (TCC 0.9.28rc) | Not yet |
| tmux | 86K | Proven, 129/129 files | Compiled, runtime bug |
| SQLite | 260K | Proven, 0.07s | Not yet |
| git | ~300K | Very likely (C) | Not yet |
| openssh | ~100K | Very likely (C) | Not yet |
| curl | ~100K | Very likely (C) | Not yet |
| openssl | ~500K | Hard (asm, intrinsics) | Not yet |
| mbedtls | ~80K | Likely (pure C) | Not yet |

**What TCC cannot compile (C++):**

| Tool | Language | Blocker |
|---|---|---|
| Node.js (V8) | C++ | Hard stop |
| Bun (JSC) | C++/Zig | Hard stop |
| Any JS engine except QuickJS | C++ | Hard stop |

**The v7p + Claude Code gap:**
Claude Code requires Bun (C++). TCC can't compile C++. QuickJS (pure C, same author as TCC, ~50K lines) could replace the JS engine, but subscription OAuth tokens are locked to official Claude Code binary. So QuickJS + custom client = API pricing only (10x more expensive).

### Option 4: Server wrapper (evaluated, rejected)

Run Claude Code on server, proxy to phone. Problem: Claude Code injects system info and executes tools locally. Making it "think" it's on the phone means faking the entire environment. Fragile, pointless complexity. The code is already git-synced to servers via `a sync`.

### Option 5: SSH to server (works today)

Phone is glass pane. `a ssh` to Mac/server, run Claude Code there with subscription pricing. Code lives on server. Results pushed via git. Already works. Already cheap. Gives up "phone as dev env" dream but economics are right.

## The Decision Tree

```
Does Termux stop dying after ADB fix?
  YES → done, keep using Termux + Claude Code
  NO  → 
    Do you need Claude Code on phone specifically?
      NO  → SSH to server (Option 5, works today)
      YES →
        Ship Bun + Claude Code in custom APK (Option 2)
        Foreground service = Android can't kill it
        Weekend project on existing APK build

    Long term regardless:
      v7p + TCC compiles bash/python/git/ssh from source
      768KB → full dev env compiled on device
      QuickJS for JS if ever needed
      Claude API (not subscription) via raw C sockets + mbedtls
```

## v7p Architecture Reference

From fredix/v7/README.md:

```
v7p binary (768KB)
  V7 kernel (7600 lines) — fs, buffer cache, proc table, shell
  libtcc (535KB)          — compile C to memory, JIT execute
  MicroPython (506KB)     — bytecode VM as function call
  LLM client (50 lines)   — raw socket to ollama HTTP API
```

Everything is function calls in one address space. No fork, no exec, no process boundary. The host kernel provides read/write/socket. V7 provides filesystem and shell.

Process model: `NPROC 16` (trivially raised), ucontext coroutines for concurrent sessions, `_proc_stacks` with 256KB per stack. Save/resume via setjmp/longjmp. Multiple shells = multiple coroutines.

Android platform layer would be ~50 lines: replace libc read/write with JNI calls to Kotlin UI.

## Key Files

- `/Users/seanpatten/a/adata/_apk_build/` — existing Android APK build (Kotlin + NDK)
- `/Users/seanpatten/fredix/v7/v7p.c` — v7p source (~10K lines, process version)
- `/Users/seanpatten/fredix/v7/README.md` — full architecture docs
- `/Users/seanpatten/a/lab/platonic_agents/ollama_agent.c` — 34-line agentic loop in C
- nanocode: https://github.com/1rgs/nanocode — 250-line Claude Code equivalent
- bun-on-termux: https://github.com/tribixbite/bun-on-termux
- bun-termux: https://github.com/Happ1ness-dev/bun-termux

## Cost Comparison

| Method | Monthly cost at heavy use |
|---|---|
| Max 5x subscription via Claude Code | $100 |
| Max 20x subscription via Claude Code | $200 |
| Direct API (Opus, no caching) | $1000-5000 |
| Direct API (Opus, 1hr cache) | $200-1000 |
| Direct API (Sonnet, 1hr cache) | $50-200 |

Subscription is the clear winner. Subscription requires real Claude Code binary. Real Claude Code binary requires Bun. Bun requires C++ runtime. C++ can't be TCC-compiled.

## Open Questions

1. Does the Termux ADB fix hold under sustained use?
2. Can Bun arm64 binary be shipped as `.so` in APK native libs and work outside Termux?
3. How much of Claude Code's JS bundle is needed for just auth + API + basic tools?
4. Could Anthropic eventually allow OAuth tokens for API access? (Feature requested: anthropics/claude-code#37205)
5. Is there a path to compile JavaScriptCore's C subset with TCC, stubbing the C++ parts?
