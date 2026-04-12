"""a bg <command...> — run command in a named tmux window, tee'd to a log. Visible to user, other agents, cross-device via tmux ls."""
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
    wrap = f"printf '{banner}\\n' | tee {log}; PYTHONUNBUFFERED=1 stdbuf -oL -eL {cmd} 2>&1 | tee -a {log}; echo; echo '=== DONE ==='; read"
    sp.run(['tmux', 'new-window', '-d', '-t', 'a:', '-n', name, wrap], check=True)
    print(f'+ tmux window: a:{name}')
    print(f'  log: {log}')
    print(f'  attach: tmux attach -t a:{name}')
    print(f'  kill:   tmux send-keys -t a:{name} C-c')

run()
