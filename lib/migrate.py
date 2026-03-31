"""a migrate - save/restore system state across OSes"""
import subprocess, sys, json, os, shutil, platform
from pathlib import Path

AROOT = Path(os.environ.get("AROOT", Path.home() / "adata"))
MIG = AROOT / "git" / "migration"
DEV = Path(os.environ.get("A_DEVICE", platform.node()))
IS_MAC = sys.platform == "darwin"
IS_ARCH = Path("/etc/arch-release").exists()
IS_DEB = Path("/etc/debian_version").exists()
HOME = Path.home()
AUTO = "-y" in sys.argv or not sys.stdin.isatty()
def run(c): return subprocess.run(c, shell=True, capture_output=True, text=True)
def has(c): return shutil.which(c) is not None
def confirm(m): return True if AUTO else input(f"{m} [y/N]: ").lower() == 'y'
def jdump(d, f): (MIG / f).write_text(json.dumps(d, indent=2))
def ldump(items, f): (MIG / f).write_text('\n'.join(items))

# ── collectors ──
def get_brew():
    f = run("brew list --formula -1").stdout.strip().split('\n')
    c = run("brew list --cask -1").stdout.strip().split('\n')
    t = run("brew tap").stdout.strip().split('\n')
    return {"formula": [x for x in f if x], "cask": [x for x in c if x], "taps": [x for x in t if x]}

def get_mas():
    if not has("mas"): return []
    return [l.strip() for l in run("mas list").stdout.strip().split('\n') if l.strip()]

def get_mac_defaults():
    """Save macOS prefs: key system settings + all third-party app plists."""
    keys = {
        "NSGlobalDomain": ["KeyRepeat", "InitialKeyRepeat", "AppleShowAllExtensions",
            "AppleInterfaceStyle", "NSAutomaticSpellingCorrectionEnabled",
            "NSAutomaticCapitalizationEnabled", "com.apple.swipescrolldirection"],
        "com.apple.dock": ["autohide", "tilesize", "orientation", "show-recents", "launchanim",
            "autohide-time-modifier", "expose-animation-duration"],
        "com.apple.universalaccess": ["reduceMotion", "reduceTransparency"],
        "com.apple.finder": ["ShowPathbar", "ShowStatusBar", "AppleShowAllFiles"],
        "com.apple.Terminal": None,  # full domain export
    }
    out = {}
    for domain, ks in keys.items():
        if ks:
            out[domain] = {}
            for k in ks:
                r = run(f"defaults read {domain} {k}")
                if r.returncode == 0: out[domain][k] = r.stdout.strip()
        else:
            r = run(f"defaults export {domain} -")
            if r.returncode == 0: out[domain] = r.stdout
    # third-party app preferences — full export
    prefs = HOME / "Library" / "Preferences"
    if prefs.exists():
        for p in prefs.glob("*.plist"):
            domain = p.stem
            if domain.startswith("com.apple.") or domain in out: continue
            r = run(f"defaults export '{domain}' -")
            if r.returncode == 0 and r.stdout.strip(): out[domain] = r.stdout
    return out

def get_mac_apps():
    """List /Applications — cask-installed vs manual."""
    apps = [a.name for a in Path("/Applications").iterdir() if a.suffix == ".app"]
    cask_names = set(run("brew list --cask -1").stdout.strip().split('\n'))
    # rough match: cask "android-studio" → "Android Studio.app"
    cask_lower = {c.replace('-',' ').lower() for c in cask_names if c}
    manual = [a for a in apps if a.replace('.app','').lower() not in cask_lower]
    return {"all": apps, "cask_managed": list(cask_names), "manual": manual}

def get_vscode():
    paths = ["code-insiders", "code",
             "/Applications/Visual Studio Code - Insiders.app/Contents/Resources/app/bin/code",
             "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"]
    for cmd in paths:
        r = run(f"'{cmd}' --list-extensions")
        if r.returncode == 0:
            exts = [e for e in r.stdout.strip().split('\n') if e]
            return {"cmd": cmd.split('/')[-1], "extensions": exts}
    return None

def get_apt_repos():
    repos, src, keys = [], Path("/etc/apt/sources.list.d"), Path("/etc/apt/keyrings")
    if not src.exists(): return repos
    for f in src.iterdir():
        if f.suffix in ['.list', '.sources'] and not f.name.endswith('.save') and not f.name.startswith('ubuntu'):
            try:
                kr = next(({"name": k.name, "data": k.read_bytes().hex()} for k in keys.glob(f"{f.stem.split('-')[0]}*.gpg")), None) if keys.exists() else None
                repos.append({"name": f.name, "content": f.read_text(), "keyring": kr})
            except: pass
    return repos

def get_apt(): r = run("apt-mark showmanual"); return sorted(r.stdout.strip().split('\n')) if r.returncode == 0 else []
def get_snap(): r = run("snap list"); return [l.split()[0] for l in r.stdout.strip().split('\n')[1:] if l] if r.returncode == 0 else []
def get_flatpak(): return [l.strip() for l in run("flatpak list --app --columns=application").stdout.split('\n') if l.strip()] if has("flatpak") else []
def get_pacman(): return run("pacman -Qqe").stdout.strip().split('\n') if has("pacman") else []
def get_gnome():
    if not has("gnome-extensions"): return None
    exts = [e for e in run("gnome-extensions list --enabled").stdout.strip().split('\n') if e]
    dconf = run("dconf dump /org/gnome/shell/").stdout
    favs = run("gsettings get org.gnome.shell favorite-apps").stdout.strip()
    return {"extensions": exts, "dconf": dconf, "favorites": favs}

def get_repos():
    repos = []
    for d in [HOME / "projects", HOME]:
        if not d.exists(): continue
        for p in d.iterdir():
            if p.is_dir() and (p / ".git").exists():
                r = run(f"git -C '{p}' remote get-url origin")
                if r.returncode == 0 and r.stdout.strip():
                    repos.append({"name": p.name, "url": r.stdout.strip(), "parent": str(p.parent)})
    return repos

# ── save ──
def save():
    MIG.mkdir(parents=True, exist_ok=True)
    dev = str(DEV)
    out = MIG / dev
    out.mkdir(exist_ok=True)
    print(f"=== Saving {dev} ({platform.system()}) ===\n")
    # temporarily point writes to device subdir
    global MIG_ORIG
    MIG_ORIG = MIG
    items = [("Git repos", get_repos, "repos.json")]
    if IS_MAC:
        items += [("Brew", get_brew, "brew.json"), ("Mac App Store", get_mas, "mas.txt"),
                  ("macOS defaults", get_mac_defaults, "defaults.json"),
                  ("Applications", get_mac_apps, "apps.json"), ("VS Code", get_vscode, "vscode.json")]
    elif IS_DEB:
        items += [("Apt repos", get_apt_repos, "apt_repos.json"), ("Apt", get_apt, "apt.txt"),
                  ("Snap", get_snap, "snap.txt"), ("Flatpak", get_flatpak, "flatpak.txt"),
                  ("GNOME", get_gnome, "gnome.json")]
    elif IS_ARCH:
        items += [("Pacman", get_pacman, "pacman.txt")]
    if not IS_MAC:
        vs = get_vscode()
        if vs: items.append(("VS Code", lambda: vs, "vscode.json"))
    for name, fn, file in items:
        try:
            data = fn() if callable(fn) else fn
            if data is None: continue
            p = out / file
            if file.endswith('.json'): p.write_text(json.dumps(data, indent=2))
            elif isinstance(data, list): p.write_text('\n'.join(str(x) for x in data))
            else: p.write_text(str(data))
            ct = len(data) if isinstance(data, (list, dict)) else 1
            print(f"  {name}: {ct}")
        except Exception as e: print(f"  {name}: error — {e}")
    print(f"\nSaved to: {out}/")

# ── restore ──
def restore(source=None):
    # find source device
    if not MIG.exists(): print("No migration data. Run 'a migrate save' first."); return
    devs = [d.name for d in MIG.iterdir() if d.is_dir() and not d.name.startswith('.')]
    if not devs: print("No saved devices."); return
    if source: src = source
    elif len(devs) == 1: src = devs[0]
    else:
        print("Available:", ', '.join(devs))
        src = input("Restore from: ").strip()
    sd = MIG / src
    if not sd.exists(): print(f"No data for {src}"); return
    print(f"\n=== Restoring from {src} → {DEV} ({platform.system()}) ===\n")

    # repos
    rf = sd / "repos.json"
    if rf.exists():
        print("1. Git repos")
        for repo in json.loads(rf.read_text()):
            dest = Path(repo.get("parent", str(HOME))) / repo["name"]
            if dest.exists(): print(f"  skip: {repo['name']}")
            else:
                slug = repo['url'].replace('https://github.com/','').replace('git@github.com:','').replace('.git','')
                print(f"  clone: {repo['name']}")
                run(f"gh repo clone {slug} '{dest}'" if has("gh") else f"git clone '{repo['url']}' '{dest}'")

    if IS_MAC: _restore_mac(sd)
    elif IS_DEB: _restore_deb(sd)
    elif IS_ARCH: _restore_arch(sd)
    _restore_vscode(sd)
    print("\n=== Done ===")

def _restore_mac(sd):
    # brew
    bf = sd / "brew.json"
    if bf.exists():
        b = json.loads(bf.read_text())
        if not has("brew"):
            print("Installing Homebrew...")
            run('/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"')
        for t in b.get("taps", []):
            if t: run(f"brew tap {t}")
        formula = b.get("formula", [])
        if formula and confirm(f"Install {len(formula)} brew formula?"):
            run(f"brew install {' '.join(formula)}")
        casks = b.get("cask", [])
        if casks and confirm(f"Install {len(casks)} casks?"):
            run(f"brew install --cask {' '.join(casks)}")

    # mas
    mf = sd / "mas.txt"
    if mf.exists() and has("mas"):
        apps = [l.split()[0] for l in mf.read_text().strip().split('\n') if l.strip()]
        if apps and confirm(f"Install {len(apps)} App Store apps?"):
            for a in apps: run(f"mas install {a}")

    # defaults
    df = sd / "defaults.json"
    if df.exists() and confirm("Apply macOS defaults (system + app settings)?"):
        data = json.loads(df.read_text())
        for domain, vals in data.items():
            if isinstance(vals, dict):
                for k, v in vals.items():
                    if v in ("0", "1"): run(f"defaults write {domain} {k} -int {v}")
                    elif v.replace('.','',1).isdigit(): run(f"defaults write {domain} {k} -float {v}")
                    else: run(f"defaults write {domain} {k} '{v}'")
            elif isinstance(vals, str) and vals.startswith("<?xml"):
                subprocess.run(f"defaults import '{domain}' -", shell=True, input=vals, text=True)
        print("  Applied. Some changes need logout to take effect.")

    # manual apps reminder
    af = sd / "apps.json"
    if af.exists():
        apps = json.loads(af.read_text())
        manual = apps.get("manual", [])
        if manual: print(f"\n  Manual apps to reinstall: {', '.join(manual)}")

def _restore_deb(sd):
    # apt repos
    rf = sd / "apt_repos.json"
    if rf.exists():
        repos = json.loads(rf.read_text())
        if repos and confirm(f"Restore {len(repos)} apt repos?"):
            for r in repos:
                if r.get('keyring'): run(f"sudo mkdir -p /etc/apt/keyrings && sudo tee /etc/apt/keyrings/{r['keyring']['name']}")
                run(f"echo '{r['content']}' | sudo tee /etc/apt/sources.list.d/{r['name']}")
            run("sudo apt update")
    # apt
    af = sd / "apt.txt"
    if af.exists():
        pkgs = [p for p in af.read_text().strip().split('\n') if p and not p.startswith('lib')]
        if pkgs and confirm(f"Install {len(pkgs)} apt packages?"):
            run(f"sudo apt install -y {' '.join(pkgs)}")
    # snap
    sf = sd / "snap.txt"
    if sf.exists():
        snaps = [s for s in sf.read_text().strip().split('\n') if s and s not in ('core','core20','core22','core24','snapd','bare')]
        if snaps and confirm(f"Install {len(snaps)} snaps?"):
            for s in snaps: run(f"sudo snap install {s}")
    # flatpak
    ff = sd / "flatpak.txt"
    if ff.exists():
        fps = [p for p in ff.read_text().strip().split('\n') if p]
        if fps and confirm(f"Install {len(fps)} flatpaks?"):
            for f in fps: run(f"flatpak install -y flathub {f}")
    # gnome
    gf = sd / "gnome.json"
    if gf.exists():
        g = json.loads(gf.read_text())
        if g.get("dconf") and confirm("Apply GNOME settings?"): run(f"dconf load /org/gnome/shell/ <<< '{g['dconf']}'")

def _restore_arch(sd):
    pf = sd / "pacman.txt"
    if pf.exists():
        pkgs = [p for p in pf.read_text().strip().split('\n') if p]
        if pkgs and confirm(f"Install {len(pkgs)} packages?"):
            run(f"sudo pacman -S --needed --noconfirm {' '.join(pkgs)}")

def _restore_vscode(sd):
    vf = sd / "vscode.json"
    if not vf.exists(): return
    v = json.loads(vf.read_text())
    cmd = v.get("cmd", "code")
    if not has(cmd): return
    exts = v.get("extensions", [])
    if exts and confirm(f"Install {len(exts)} VS Code extensions?"):
        for e in exts: run(f"{cmd} --install-extension {e}")

# ── list ──
def ls():
    if not MIG.exists(): print("No migration data."); return
    for d in sorted(MIG.iterdir()):
        if d.is_dir() and not d.name.startswith('.'):
            files = list(d.iterdir())
            print(f"  {d.name}: {len(files)} files — {', '.join(f.name for f in files)}")

if __name__ == "__main__":
    # called as: python3 migrate.py migrate <cmd> OR python3 migrate.py <cmd>
    args = sys.argv[1:]
    if args and args[0] == "migrate": args = args[1:]
    cmd = args[0] if args else ""
    src = args[1] if len(args) > 1 else None
    if cmd in ("save", "s", "get_info"): save()
    elif cmd in ("restore", "r", "apply"): restore(src)
    elif cmd in ("ls", "list", "l"): ls()
    else: print("Usage: a migrate [save|restore|ls]")
