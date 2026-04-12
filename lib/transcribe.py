"""a transcribe <file|folder|log> — faster-whisper + VAD. Manual file specification is brittle: filename date != upload date, mod times drift, glob patterns over rclone listings produce surprising matches. Pass exact paths only."""
import sys, os, subprocess as sp, time, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from _common import ADATA_ROOT

OUT = str(ADATA_ROOT / 'transcripts')
TMP = str(ADATA_ROOT / 'local' / 'tmp')
LOG = str(ADATA_ROOT / 'git' / 'transcribe_log.jsonl')
EXTS = ('.flac', '.wav', '.m4a', '.mp3', '.ogg', '.opus', '.3gp', '.amr')
MODEL, BACKEND = 'small', 'faster-whisper int8 cpu_threads=20 vad_filter=True'

def ensure_deps():
    try: from faster_whisper import WhisperModel; return
    except ImportError: pass
    sp.run(['pip', 'install', '--user', 'faster-whisper'], check=True)

_M = None
def transcribe(path, model='small'):
    global _M
    if _M is None:
        from faster_whisper import WhisperModel
        _M = WhisperModel(model, device='cpu', compute_type='int8', cpu_threads=20)
    t = time.time()
    segs, _ = _M.transcribe(path, language='en', condition_on_previous_text=False, vad_filter=True)
    return ' '.join(s.text.strip() for s in segs).strip(), time.time() - t

def is_remote(p): return ':' in p and not p.startswith('/')

def log_done():
    if not os.path.exists(LOG): return set()
    return {json.loads(l)['src'] for l in open(LOG) if l.strip()}

def log_write(rec):
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    open(LOG, 'a').write(json.dumps(rec) + '\n')

def list_remote(p):
    r = sp.run(['rclone', 'lsf', p], capture_output=True, text=True)
    return [f.strip() for f in r.stdout.strip().split('\n') if f.strip().lower().endswith(EXTS)]

def process(target, done=None):
    if done and target in done: print(f'skip (logged): {target}'); return
    if is_remote(target):
        remote_dir = target.rsplit('/', 1)[0] if '/' in target else target
        lp = os.path.join(TMP, os.path.basename(target))
        print(f'Downloading {target}...')
        sp.run(['rclone', 'copy', target, TMP], check=True)
    else: lp = target; remote_dir = None
    sz = os.path.getsize(lp)
    print(f'Transcribing {os.path.basename(lp)} ({sz/1e6:.1f}MB)...')
    txt, dur = transcribe(lp)
    rec = {'ts': time.strftime('%Y-%m-%dT%H:%M:%S'), 'src': target, 'bytes': sz, 'proc_s': round(dur,1), 'chars': len(txt), 'model': MODEL, 'backend': BACKEND, 'preview': txt[:200]}
    if txt:
        base = os.path.splitext(os.path.basename(lp))[0]
        out = os.path.join(OUT, base + '.txt')
        open(out, 'w').write(txt + '\n')
        print(f'{dur:.0f}s | {len(txt)} chars | {out}')
        if remote_dir:
            sp.run(['rclone', 'copy', out, remote_dir], check=True)
            print(f'uploaded -> {remote_dir}/{base}.txt')
        print(txt[:500])
    else: print('(no speech detected)')
    log_write(rec)
    if remote_dir and lp.startswith(TMP):
        os.remove(lp); print(f'deleted local: {lp}\n')

def run():
    os.makedirs(OUT, exist_ok=True); os.makedirs(TMP, exist_ok=True)
    ensure_deps()
    args = sys.argv[2:]
    if not args:
        print('Usage: a transcribe <file|folder|log>\n  file:   local path or remote:path/file.flac\n  folder: local dir or remote:path (transcribes every audio file inside, skips already-logged)\n  log:    show transcription history (timing, char counts, empties, software version)')
        return
    if args[0] == 'log':
        if not os.path.exists(LOG): print('(no log)'); return
        recs = [json.loads(l) for l in open(LOG) if l.strip()]
        ok = sum(1 for r in recs if r['chars'] > 0); empty = len(recs) - ok
        tot_b = sum(r['bytes'] for r in recs); tot_s = sum(r['proc_s'] for r in recs)
        print(f'{len(recs)} runs | {ok} ok | {empty} empty | {tot_b/1e9:.1f}GB | {tot_s/60:.1f}min proc')
        backends = {r['backend'] for r in recs}
        for b in backends: print(f'  backend: {b}')
        print()
        for r in recs[-20:]:
            mark = '✓' if r['chars'] else '∅'
            print(f"  {mark} {r['ts']} {r['proc_s']:>6.1f}s {r['chars']:>6}ch  {r['src'].rsplit('/',1)[-1][:50]}")
            if r['chars']: print(f"      {r['preview'][:120]}")
        return
    target = ' '.join(args)
    done = log_done()
    if is_remote(target):
        if target.lower().endswith(EXTS): process(target, done)
        else:
            files = list_remote(target)
            print(f'{len(files)} audio files in {target}')
            for f in files: process(f'{target}/{f}', done)
    elif os.path.isdir(target):
        files = sorted(f for f in os.listdir(target) if f.lower().endswith(EXTS))
        print(f'{len(files)} audio files in {target}')
        for f in files: process(os.path.join(target, f), done)
    else: process(target, done)

run()
