"""a bg <command...> — run command in a named tmux window, tee'd to a log. Visible to user, other agents, cross-device via tmux ls.
Completion is a tmux wait-for signal (not grep, not polling): the wrapper fires `tmux wait-for -S d-<name>` after the cmd exits,
then sleeps on `read` so the window persists for human/agent inspection. Any number of LLMs or humans can block on the same signal
(`tmux wait-for d-<name>`) and all get unblocked simultaneously — one pty session, many concurrent watchers, zero single-owner
semantics. The pane is not consumed by the watcher, so `a bg` works as a shared event bus instead of a single-agent job tool.
For memory-risky jobs (ML model loads, gemma, training) wrap to cap RAM and avoid system-wide OOM thrash:
  Linux: a bg systemd-run --user --scope --property=MemoryMax=8G --property=MemorySwapMax=0 ./job
  macOS: no cgroup equivalent; use  ulimit -v 8000000; ./job  or run in Docker. Mac's jetsam handles OOM reactively, less precise than Linux cgroups."""
import sys, os, subprocess as sp, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from _common import ADATA_ROOT

LOGS = str(ADATA_ROOT / 'local' / 'bg')

def run():
    args = sys.argv[2:]
    if not args:
        sp.run(['tmux', 'list-windows', '-t', 'a', '-F', '#I #W #{window_activity_string}'])
        print(f'\nlogs: {LOGS}/')
        print('Usage: a bg <command...>\n  a bg transcribe new\n  a bg "long running thing"')
        return
    os.makedirs(LOGS, exist_ok=True)
    cmd = 'a ' + ' '.join(args)
    first = args[0].replace('/', '_')
    ts = time.strftime('%H%M')
    name = f'{first}-{ts}'
    log = f'{LOGS}/{name}.log'
    full_ts = time.strftime('%Y-%m-%d %H:%M:%S')
    banner = f"=== a bg: {name} ===\\nstart: {full_ts}\\ncmd:   {cmd}\\nlog:   {log}\\n--- starting ---"
    sig = f'd-{name}'
    wrap = f"printf '{banner}\\n' | tee {log}; PYTHONUNBUFFERED=1 stdbuf -oL -eL {cmd} 2>&1 | tee -a {log}; echo; echo '=== DONE ==='; tmux wait-for -S {sig}; read"
    sp.run(['tmux', 'new-window', '-d', '-t', 'a:', '-n', name, wrap], check=True)
    print(f'+ tmux window: a:{name} (blocking until done; window persists)')
    print(f'  log: {log}')
    sp.run(['tmux', 'wait-for', sig])
    print('=== DONE ===')

run()
