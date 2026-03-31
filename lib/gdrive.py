"""aio gdrive — Google Drive file operations"""
import sys, os, subprocess as sp, json
from _common import cloud_login, cloud_logout, cloud_sync, cloud_status, _configured_remotes, RCLONE_BACKUP_PATH, DATA_DIR, SYNC_ROOT, DEVICE_ID, alog, get_rclone
from sync import cloud_sync as cloud_sync_tar

_AF = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'adata', 'git', 'gdrive_accounts.txt')

def _accts():
    if not os.path.exists(_AF): return []
    return [{'n': p[0], 'r': p[1], 'd': p[2] if len(p) > 2 else ''} for line in open(_AF) if len(p := line.strip().split(None, 2)) >= 2]

def _resolve(name):
    for a in _accts():
        if name in (a['n'], a['r']): return a['r']
    for a in _accts():
        if name.lower() in a['n'].lower(): return a['r']
    return name if name in _configured_remotes() else None

def _tgt(s):
    return s.split(':', 1) if ':' in s else (s, '')

def _rp(name, path):
    r = _resolve(name)
    if not r: print(f"x Unknown '{name}'"); return None
    return f'{r}:{path}'

def _rclone(args, **kw):
    rc = get_rclone()
    if not rc: print("x rclone not found"); return None
    return sp.run([rc] + args, **kw)

def _target_or_bail(args, usage):
    if not args: print(usage); return None, None
    n, p = _tgt(args[0])
    rp = _rp(n, p)
    return rp, args[1:]

def cmd_cp(a):
    if len(a) < 2: print("a gdrive cp <local> <account:dest>"); return
    if not os.path.exists(a[0]): print(f"x {a[0]} not found"); return
    n, p = _tgt(a[1]); rp = _rp(n, p)
    if not rp: return
    print(f"→ {a[0]} → {rp}")
    r = _rclone(['copy' if os.path.isdir(a[0]) else 'copyto', a[0], rp, '-P'])
    print("✓" if r and r.returncode == 0 else "x Failed")

def cmd_mv(a):
    if len(a) < 2: print("a gdrive mv <local> <account:dest>"); return
    if not os.path.exists(a[0]): print(f"x {a[0]} not found"); return
    n, p = _tgt(a[1]); rp = _rp(n, p)
    if not rp: return
    print(f"→ {a[0]} → {rp}")
    r = _rclone(['move', a[0], rp, '-P', '--delete-empty-src-dirs'])
    print("✓" if r and r.returncode == 0 else "x Failed")

def cmd_get(a):
    if len(a) < 2: print("a gdrive get <account:path> <local>"); return
    n, p = _tgt(a[0]); rp = _rp(n, p)
    if not rp: return
    print(f"← {rp} → {a[1]}")
    r = _rclone(['copy', rp, a[1], '-P'])
    print("✓" if r and r.returncode == 0 else "x Failed")

def cmd_ls(a):
    rp, _ = _target_or_bail(a, "a gdrive ls <account:path>")
    if not rp: return
    r1 = _rclone(['lsd', rp], capture_output=True, text=True)
    r2 = _rclone(['lsf', rp, '--files-only'], capture_output=True, text=True)
    dirs = [l.split()[-1] for l in (r1.stdout if r1 else '').strip().splitlines() if l.strip()]
    files = (r2.stdout if r2 else '').strip().splitlines()
    if not dirs and not files: print("  (empty)"); return
    for d in dirs: print(f"  {d}/")
    for f in files: print(f"  {f}")
    print(f"\n  {len(dirs)} folders, {len(files)} files")

def cmd_tree(a):
    rp, _ = _target_or_bail(a, "a gdrive tree <account:path>")
    if not rp: return
    r = _rclone(['lsf', rp, '-R'], capture_output=True, text=True)
    print(r.stdout if r and r.stdout else "  (empty)")

def cmd_find(a):
    if len(a) < 2: print("a gdrive find <account:path> <glob>"); return
    n, p = _tgt(a[0]); rp = _rp(n, p)
    if not rp: return
    r = _rclone(['lsf', rp, '-R', '--include', a[1]], capture_output=True, text=True)
    lines = (r.stdout if r else '').strip().splitlines()
    for l in lines: print(f"  {l}")
    print(f"  {len(lines)} matches" if lines else "  No matches")

def cmd_size(a):
    rp, _ = _target_or_bail(a, "a gdrive size <account:path>")
    if rp: _rclone(['size', rp])

def cmd_rm(a):
    rp, _ = _target_or_bail(a, "a gdrive rm <account:path>")
    if not rp: return
    print(f"Delete {rp}? [y/N] ", end='', flush=True)
    if input().strip().lower() != 'y': print("Cancelled"); return
    r = _rclone(['purge', rp])
    print("✓" if r and r.returncode == 0 else "x Failed")

def cmd_link(a):
    rp, _ = _target_or_bail(a, "a gdrive link <account:path>")
    if not rp: return
    r = _rclone(['link', rp], capture_output=True, text=True)
    print(r.stdout.strip() if r and r.returncode == 0 and r.stdout.strip() else "x No link")

def _pull_auth():
    rem = _configured_remotes(); rem or (print("Login first: a gdrive login"), exit(1))
    for f, d in [('hosts.yml', '~/.config/gh'), ('rclone.conf', '~/.config/rclone')]:
        os.makedirs(os.path.expanduser(d), exist_ok=True); sp.run(['rclone', 'copy', f'{rem[0]}:{RCLONE_BACKUP_PATH}/backup/auth/{f}', os.path.expanduser(d), '-q'])
    open(f"{DATA_DIR}/.auth_shared","w").close(); os.path.exists(f"{DATA_DIR}/.auth_local") and os.remove(f"{DATA_DIR}/.auth_local"); print("✓ Auth synced")

_C = {'cp': cmd_cp, 'mv': cmd_mv, 'get': cmd_get, 'ls': cmd_ls, 'tree': cmd_tree,
      'find': cmd_find, 'size': cmd_size, 'rm': cmd_rm, 'link': cmd_link}

def run():
    a = sys.argv[1:]
    if a and a[0] in ('gdrive', 'dummy'): a = a[1:]
    cmd, rest = (a[0], a[1:]) if a else (None, [])
    if cmd in _C: _C[cmd](rest)
    elif cmd == 'login': cloud_login(custom=(rest[0] if rest else '') == 'custom')
    elif cmd == 'logout': cloud_logout()
    elif cmd == 'sync':
        cloud_sync(wait=True); ok, msg = cloud_sync_tar(str(SYNC_ROOT), 'git')
        print(f"✓ Synced ({msg})"); ok and alog(f"gdrive sync")
    elif cmd == 'init': _pull_auth()
    else:
        print("a gdrive — Google Drive\n")
        for x in _accts():
            r = _rclone(['about', f'{x["r"]}:', '--json'], capture_output=True, text=True)
            if r and r.returncode == 0:
                d = json.loads(r.stdout); u, t = d.get('used',0)/(1024**3), d.get('total',0)/(1024**3)
                print(f"  {x['n']:20s} {u:.1f}G / {t:.0f}G  {x['d']}")
            else: print(f"  {x['n']:20s} (error)")
        print("""
  cp <local> <acct:dest>        Copy to gdrive
  mv <local> <acct:dest>        Move to gdrive
  get <acct:path> <local>       Download from gdrive
  ls <acct:path>                List contents
  tree <acct:path>              Recursive listing
  find <acct:path> <glob>       Search by pattern
  size <acct:path>              Show size
  rm <acct:path>                Delete (confirms)
  link <acct:path>              Shareable link
  sync                          Backup all to gdrive
  login [custom]                Add account
  logout                        Remove account
  init                          Pull auth (new device)

  Paths: account:folder/file  Fuzzy: 'jared' → 'jaredtwo2two'""")
run()
