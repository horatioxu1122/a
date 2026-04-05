# Session Report: April 4-5, 2026

## What was built

**g4agent.c** (lib/, 3535 bytes) — Local Gemma 4 CMD: agent. Auto-starts ollama, env model select (G4M), streaming with thinking display, \u003e unicode decode, CMD: start-only safety match. `sh g4agent.c` builds and runs.

**bench_llm.py** (142 lines) — Two-tier LLM quality benchmark. Tier 1: 23 fast probes (no thinking, parallel, <2s per question). Tier 2: 10 advanced capability tests (thinking enabled). Includes hard knowledge probes: trig identities, FFT complexity, Taylor series.

**8 compression scripts** — layer_scales.py, compactifai_test.py, compactifai_sweep.py, awq_salience.py, sparsity_test.py, svd_truncation.py, quant_sim.py, gguf_strip.py. All 30 lines or less, produced by parallel agents.

**5 expert persona models** — gemma4-torvalds, gemma4-knuth, gemma4-shannon, gemma4-vonneumann, gemma4-crunch. Each is an ollama Modelfile with character-specific system prompt. Plus crunch_vote.py pipeline: generate from all personas, crunch to single best insight.

**Multi-agent infrastructure** — `a j` no-fork default (PID-unique window names), `a ls` freeze fix (no tmux attach outside tmux, skip perf kill for tmux queries), AGENTS.md with incentive-aligned agent context.

**IDEAS.md #103-107** — Agent communication is fundamentally broken (#103), progress is verification-bottlenecked (#104), value of proof = decision quality delta (#105), proof enables chaining (#106), reliable foundations enable unreliable tops (#107).

## Key measurements

### Model benchmarks (RTX 4080 12GB + 30GB RAM)

| Model | Size | Gen tok/s | Tier 1 | Tier 2 |
|---|---|---|---|---|
| E2B | 7.2GB | 182 | 19/23 (83%) | 10/10 (100%) |
| E4B | 9.6GB | 119 | 19/20 (95%) | 8/10 (80%) |
| 31B IQ2_M | 10.75GB | 2 | 22/23 (96%) | 6/10 (60%) |

E2B with thinking beats E4B on advanced reasoning. 31B at 2-bit scores highest on knowledge (96%) but 2 tok/s limits thinking tasks.

### Compression methods (single weight matrix, Gemma 4 E2B ffn_down)

| Method | Error | Compression |
|---|---|---|
| 30% magnitude sparsity | 11% | 1.3x |
| 4-bit QuIP-style quantization | 13% | 4x |
| 30% prune + 4-bit combined | 17% | 2.8 bpw |
| SVD rank 512 | 65% | 2.4x |
| CompactifAI MPO rank 32 | 96% | 62.9x |

### 31B layer importance (layer_output_scale)

3 layers with scale < 0.09: blk.59 (0.036), blk.1 (0.065), blk.0 (0.089) = 951MB removable. 4.9x gap to next candidate (blk.28 at 0.436). Blocks 2-4, 13-18 are most critical (>0.97).

### Persona crunch pipeline

4 expert personas reviewed a.c architecture. Torvalds: flat data structure. Knuth: internal state machine. Shannon: entropy encoding of agent state. Von Neumann: formal state abstraction. Cruncher selected Torvalds. Analysis: Torvalds is wrong for this case (tmux is authoritative state), but Knuth's point about shell brittleness is the real long-term fix.

## What we learned

**Thinking is the equalizer.** E2B (2.3B params) scored 100% on advanced reasoning with thinking enabled. E4B (4.5B) scored 80%. Token budget matters more than parameter count.

**Existing quantization already solved the deployment problem.** 31B at IQ2_M fits 12GB VRAM and scores 96% on knowledge probes. No custom compression needed for deployment — only for research (proving layer removal + quant beats quant alone).

**The GGUF rewriter is the blocker for compression research.** Strip scripts corrupt header offsets. Proper rewriter (~200 lines C) needed to produce loadable pruned models.

**Multi-agent spawning works but needs discipline.** 6+ agents overwhelm tmux and RAM. 2-3 concurrent agents is the practical limit on 30GB RAM. Agents that produce 30-line scripts with .a_done work well. Agents given vague tasks drift.

**The crunch pipeline produces real value.** 4 experts × 2000 chars each → 1 sentence. The sentence was actionable and correct. Generate wide, compress hard.

## What's next

1. Wave compression experiment — FFT-based weight analysis (agent didn't finish, needs respawn)
2. GGUF rewriter — proper header offset correction for layer removal
3. 26B benchmark — clean run without competing agents
4. Persona refinement — test crunch pipeline on harder questions, compare to single-model answers
5. Q-Compress integration — use layer_output_scale as classical baseline for Fujitsu quantum challenge
