"""aio gdrive - Google Drive file operations"""
import sys, os, subprocess as sp
from _common import cloud_login, cloud_logout, cloud_sync, cloud_status, _configured_remotes, RCLONE_BACKUP_PATH, DATA_DIR, SYNC_ROOT, DEVICE_ID, alog, get_rclone
from sync import cloud_sync as cloud_sync_tar

ACCOUNTS_FILE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'adata', 'git', 'gdrive_accounts.txt')

def _load_accounts():
    accts = []
    if not os.path.exists(ACCOUNTS_FILE): return accts
    for line in open(ACCOUNTS_FILE):
        parts = line.strip().split(None, 2)
        if len(parts) >= 2:
            accts.append({'name': parts[0], 'remote': parts[1], 'desc': parts[2] if len(parts) > 2 else ''})
    return accts

def _resolve(name):
    for a in _load_accounts():
        if name == a['name'] or name == a['remote']: return a['remote']
    for a in _load_accounts():
        if name.lower() in a['name'].lower(): return a['remote']
    if name in [r for r in _configured_remotes()]: return name
    return None

def _parse_target(s):
    if ':' in s:
        name, path = s.split(':', 1)
        return name, path
    return s, ''

def _rc():
    r = get_rclone()
    if not r: print("x rclone not found")
    return r

def _rpath(name, path):
    remote = _resolve(name)
    if not remote:
        print(f"x Unknown account '{name}'")
        _show_accounts()
        return None
    return f'{remote}:{path}'

def _show_accounts():
    accts = _load_accounts()
    if not accts:
        print("  No accounts. Run: a gdrive login")
        return
    print("\nAccounts:")
    for a in accts:
        print(f"  {a['name']:20s} {a['desc']}")

def _about():
    rc = _rc()
    if not rc: return
    accts = _load_accounts()
    if not accts: _show_accounts(); return
    for a in accts:
        r = sp.run([rc, 'about', f'{a["remote"]}:', '--json'], capture_output=True, text=True)
        if r.returncode == 0:
            import json
            d = json.loads(r.stdout)
            used = d.get('used', 0) / (1024**3)
            total = d.get('total', 0) / (1024**3)
            free = d.get('free', 0) / (1024**3)
            print(f"  {a['name']:20s} {used:.1f}G / {total:.0f}G ({free:.0f}G free)  {a['desc']}")
        else:
            print(f"  {a['name']:20s} (error)")

def cmd_cp(args):
    """Copy local file/folder to gdrive."""
    if len(args) < 2:
        print("Usage: a gdrive cp <local> <account:dest>"); return
    local, target = args[0], args[1]
    if not os.path.exists(local): print(f"x {local} not found"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(target)
    rdest = _rpath(name, path)
    if not rdest: return
    op = 'copy' if os.path.isdir(local) else 'copyto'
    print(f"→ {local} → {rdest}")
    r = sp.run([rc, op, local, rdest, '-P'])
    print(f"✓ Done" if r.returncode == 0 else f"x Failed")

def cmd_mv(args):
    """Move local file/folder to gdrive (deletes local after upload)."""
    if len(args) < 2:
        print("Usage: a gdrive mv <local> <account:dest>"); return
    local, target = args[0], args[1]
    if not os.path.exists(local): print(f"x {local} not found"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(target)
    rdest = _rpath(name, path)
    if not rdest: return
    op = 'move'
    print(f"→ {local} → {rdest}")
    r = sp.run([rc, op, local, rdest, '-P', '--delete-empty-src-dirs'])
    print(f"✓ Moved" if r.returncode == 0 else f"x Failed")

def cmd_get(args):
    """Download file/folder from gdrive to local."""
    if len(args) < 2:
        print("Usage: a gdrive get <account:path> <local_dest>"); return
    target, local = args[0], args[1]
    rc = _rc()
    if not rc: return
    name, path = _parse_target(target)
    rsrc = _rpath(name, path)
    if not rsrc: return
    print(f"← {rsrc} → {local}")
    r = sp.run([rc, 'copy', rsrc, local, '-P'])
    print(f"✓ Done" if r.returncode == 0 else f"x Failed")

def cmd_ls(args):
    """List folders and files at a gdrive path."""
    if not args:
        print("Usage: a gdrive ls <account:path>"); _show_accounts(); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    # show folders then files
    r1 = sp.run([rc, 'lsd', rp], capture_output=True, text=True)
    r2 = sp.run([rc, 'lsf', rp, '--files-only'], capture_output=True, text=True)
    folders = [l.split()[-1] for l in r1.stdout.strip().splitlines() if l.strip()] if r1.stdout.strip() else []
    files = r2.stdout.strip().splitlines() if r2.stdout.strip() else []
    if not folders and not files:
        print(f"  (empty or not found)"); return
    for f in folders: print(f"  {f}/")
    for f in files: print(f"  {f}")
    print(f"\n  {len(folders)} folders, {len(files)} files")

def cmd_tree(args):
    """Show recursive tree of a gdrive path."""
    if not args:
        print("Usage: a gdrive tree <account:path>"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    r = sp.run([rc, 'tree', rp], capture_output=True, text=True)
    if r.returncode == 0:
        print(r.stdout)
    else:
        # fallback: lsf recursive
        r2 = sp.run([rc, 'lsf', rp, '-R'], capture_output=True, text=True)
        print(r2.stdout if r2.stdout else "  (empty)")

def cmd_find(args):
    """Search for files by name pattern across a gdrive account."""
    if len(args) < 2:
        print("Usage: a gdrive find <account:path> <pattern>")
        print("  Pattern is a glob, e.g. *.pdf, report*, **/*.md"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    pattern = args[1]
    print(f"Searching {rp} for '{pattern}'...")
    r = sp.run([rc, 'lsf', rp, '-R', '--include', pattern], capture_output=True, text=True)
    if r.stdout.strip():
        lines = r.stdout.strip().splitlines()
        for l in lines: print(f"  {l}")
        print(f"\n  {len(lines)} matches")
    else:
        print("  No matches")

def cmd_size(args):
    """Show size of a gdrive path."""
    if not args:
        print("Usage: a gdrive size <account:path>"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    sp.run([rc, 'size', rp])

def cmd_rm(args):
    """Delete a file or folder on gdrive."""
    if not args:
        print("Usage: a gdrive rm <account:path>"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    print(f"Delete {rp}? [y/N] ", end='', flush=True)
    if input().strip().lower() != 'y': print("Cancelled"); return
    r = sp.run([rc, 'purge', rp])
    print(f"✓ Deleted" if r.returncode == 0 else f"x Failed")

def cmd_link(args):
    """Get shareable link for a gdrive file."""
    if not args:
        print("Usage: a gdrive link <account:path>"); return
    rc = _rc()
    if not rc: return
    name, path = _parse_target(args[0])
    rp = _rpath(name, path)
    if not rp: return
    r = sp.run([rc, 'link', rp], capture_output=True, text=True)
    if r.returncode == 0 and r.stdout.strip():
        print(r.stdout.strip())
    else:
        print(f"x Could not get link ({r.stderr.strip()})")

def _pull_auth():
    rem = _configured_remotes(); rem or (print("Login first: aio gdrive login"), exit(1))
    for f, d in [('hosts.yml', '~/.config/gh'), ('rclone.conf', '~/.config/rclone')]:
        os.makedirs(os.path.expanduser(d), exist_ok=True); sp.run(['rclone', 'copy', f'{rem[0]}:{RCLONE_BACKUP_PATH}/backup/auth/{f}', os.path.expanduser(d), '-q'])
    open(f"{DATA_DIR}/.auth_shared","w").close(); os.path.exists(f"{DATA_DIR}/.auth_local") and os.remove(f"{DATA_DIR}/.auth_local"); print("✓ Auth synced (shared)")

CMDS = {
    'cp':    (cmd_cp,    '<local> <account:dest>',      'Copy file/folder to gdrive'),
    'mv':    (cmd_mv,    '<local> <account:dest>',      'Move file/folder to gdrive (deletes local)'),
    'get':   (cmd_get,   '<account:path> <local_dest>',  'Download from gdrive to local'),
    'ls':    (cmd_ls,    '<account:path>',               'List folder contents'),
    'tree':  (cmd_tree,  '<account:path>',               'Recursive file listing'),
    'find':  (cmd_find,  '<account:path> <pattern>',     'Search files by glob pattern'),
    'size':  (cmd_size,  '<account:path>',               'Show total size of path'),
    'rm':    (cmd_rm,    '<account:path>',               'Delete file/folder (confirms first)'),
    'link':  (cmd_link,  '<account:path>',               'Get shareable link'),
    'sync':  (None,      '',                             'Backup data + auth + git to all accounts'),
    'login': (None,      '[custom]',                     'Add gdrive account'),
    'logout':(None,      '',                             'Remove last account'),
    'init':  (None,      '',                             'Pull auth from gdrive (new device setup)'),
}

def run():
    wda = sys.argv[2] if len(sys.argv) > 2 else None
    rest = sys.argv[3:] if len(sys.argv) > 3 else []

    if wda in CMDS and CMDS[wda][0]:
        CMDS[wda][0](rest)
    elif wda == 'login': cloud_login(custom=(rest[0] if rest else '') == 'custom')
    elif wda == 'logout': cloud_logout()
    elif wda == 'sync':
        cloud_sync(wait=True)
        ok, msg = cloud_sync_tar(str(SYNC_ROOT), 'git')
        print(f"✓ Synced data + auth + git ({msg}) to GDrive")
        if ok: alog(f"gdrive sync → gdrive:{RCLONE_BACKUP_PATH}/backup/data/ + auth/ + {DEVICE_ID}/git.tar.zst")
    elif wda == 'init': _pull_auth()
    else:
        print("a gdrive — Google Drive file operations\n")
        _about()
        print("\nCommands:")
        for name, (_, usage, desc) in CMDS.items():
            u = f' {usage}' if usage else ''
            print(f"  a gdrive {name}{u}")
            print(f"    {desc}")
        print(f"\nAll paths use account:path format. Account names from adata/git/gdrive_accounts.txt")
        print(f"Fuzzy match: 'jared' resolves to 'jaredtwo2two'\n")
        print(f"Examples:")
        print(f"  a gdrive ls jaredtwo2two:")
        print(f"  a gdrive cp report.md jaredtwo2two:research/")
        print(f"  a gdrive find seanpatten: '*.pdf'")
        print(f"  a gdrive get jaredtwo2two:notes/file.md ./")
        print(f"  a gdrive tree jaredtwo2two:quantum fusion shared /")
run()
