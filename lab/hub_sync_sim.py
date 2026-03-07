#!/usr/bin/env python3
"""Monte Carlo sim of hub_save append-only file accumulation.
Models the real death spiral observed 2026-03-06 on HSU:
  - testjob2 (*:0/5) generated ~288 timestamped files/day
  - git add/commit/push latency scaled with untracked file count
  - sync kept failing (fetch first) because operations took longer
    than the interval between new file creation
  - 597 untracked files found, sync completely jammed
  - All 24 hub timers were firing fine, just couldn't push state
Fix: rm old {name}_*.txt before writing new timestamped file.
This sim validates the fix prevents accumulation and death spiral."""
import multiprocessing as mp, os, signal, time, random, statistics

TIMEOUT = 1
RUNS = 5000
DAYS = 7
TICKS = 1440 * DAYS

DEVICES = ["HSU", "ubuntu", "pixel", "mac"]
JOBS = [
    ("testjob2", 5, "HSU"),
    ("g-musk-scan", 30, "HSU"),
    ("sync", 30, "ubuntu"),
    ("g-joke", 1440, "HSU"),
    ("g-demis", 1440, "HSU"),
    ("g-sp500", 1440, "HSU"),
    ("g-weather", 1440, "HSU"),
    ("g-aqi", 1440, "HSU"),
    ("g-lmsys", 1440, "HSU"),
    ("g-huang", 1440, "HSU"),
    ("g-frontier", 1440, "HSU"),
    ("g-musk", 1440, "HSU"),
    ("g-reminder", 1440, "HSU"),
    ("g-managers", 1440, "HSU"),
    ("g-research", 1440, "HSU"),
    ("g-motivate", 1440, "HSU"),
    ("daily-work", 1440, "HSU"),
    ("gc-collab", 1440, "HSU"),
    ("gc-notes", 1440, "HSU"),
    ("gc-worktree", 1440, "HSU"),
    ("g-androidtv", 1440, "HSU"),
    ("g-mgr-scan", 720, "HSU"),
    ("g-frt-scan", 720, "HSU"),
    ("compress", 1440, "ubuntu"),
    ("bench", 1440, "ubuntu"),
]
ACTIVITY_PER_RUN = 2
SYNC_INTERVAL = 30
GIT_BASE_LATENCY = 1.0
GIT_PER_FILE_LATENCY = 0.006
RACE_WINDOW = 4.0

# precompute: per-tick job firings as (job_idx, dev_idx)
_dev_idx = {d: i for i, d in enumerate(DEVICES)}
_sched = [[] for _ in range(TICKS)]
for ji, (_, freq, dev) in enumerate(JOBS):
    di = _dev_idx[dev]
    for t in range(0, TICKS, freq):
        _sched[t].append((ji, di))
ND, NJ = len(DEVICES), len(JOBS)
TOTAL_SYNCS_PER_RUN = (TICKS // SYNC_INTERVAL) * ND

def sim_batch(args):
    count, cleanup = args
    r_af, r_act, r_fs, r_pk = [], [], [], []
    for _ in range(count):
        agent = [[0]*NJ for _ in range(ND)]
        act = [0]*ND
        peak = fails = 0
        for t in range(TICKS):
            for ji, di in _sched[t]:
                if cleanup:
                    agent[di][ji] = 0
                agent[di][ji] += 1
                act[di] += ACTIVITY_PER_RUN
            if t % SYNC_INTERVAL == 0:
                for di in range(ND):
                    n = sum(agent[di]) + act[di]
                    if n > peak: peak = n
                    lat = GIT_BASE_LATENCY + n * GIT_PER_FILE_LATENCY
                    if lat > RACE_WINDOW or random.random() < 0.05:
                        fails += 1
                    else:
                        act[di] = 0
        r_af.append(sum(sum(r) for r in agent))
        r_act.append(sum(act))
        r_fs.append(fails)
        r_pk.append(peak)
    return r_af, r_act, r_fs, r_pk

def run(label, cleanup):
    t0 = time.monotonic()
    ncpu = os.cpu_count() or 4
    batch = max(1, RUNS // ncpu)
    tasks = [(batch, cleanup)] * ncpu
    ctx = mp.get_context("fork")
    with ctx.Pool(ncpu) as pool:
        results = pool.map(sim_batch, tasks)
    elapsed = time.monotonic() - t0
    if elapsed > TIMEOUT:
        print(f"PERF KILL: {label} took {elapsed:.1f}s > {TIMEOUT}s limit")
        os.kill(os.getpid(), signal.SIGKILL)
    af, ac, fs, pk = [], [], [], []
    for r in results:
        af.extend(r[0]); ac.extend(r[1]); fs.extend(r[2]); pk.extend(r[3])
    print(f"{label}:")
    print(f"  agent config files: {statistics.mean(af):.0f}")
    print(f"  activity files:     {statistics.mean(ac):.0f}")
    print(f"  total untracked:    {statistics.mean(af)+statistics.mean(ac):.0f}")
    print(f"  peak untracked:     {statistics.mean(pk):.0f}")
    fr = statistics.mean(fs)/TOTAL_SYNCS_PER_RUN*100
    print(f"  sync failures:      {statistics.mean(fs):.1f}/{TOTAL_SYNCS_PER_RUN} ({fr:.1f}%)")
    print(f"  (real HSU: 3586 agent + ~600 activity, {DAYS}d sim)")
    print(f"  [{elapsed:.2f}s on {ncpu} cores]\n")

if __name__ == "__main__":
    print(f"Hub sync Monte Carlo ({RUNS} runs, {DAYS} days, {TIMEOUT}s perf limit)\n")
    print("Simulates death spiral observed 2026-03-06:")
    print("  git latency scales with file count, causing push races,")
    print("  causing accumulation, causing more latency...\n")
    run("NO cleanup (old behavior)", False)
    run("WITH cleanup (new behavior)", True)
