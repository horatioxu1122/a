"""a transcribe [gdrive_path|local_file] — faster-whisper + VAD. 27x faster than openai-whisper on sparse audio."""
import sys, os, subprocess as sp, time, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from _common import ADATA_ROOT

OUT = str(ADATA_ROOT / 'transcripts')
TMP = str(ADATA_ROOT / 'local' / 'tmp')
REMOTE = 'a-gdrive2:Archive/Files/Easy Voice Recorder'

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

def run():
    os.makedirs(OUT, exist_ok=True); os.makedirs(TMP, exist_ok=True)
    ensure_deps()
    args = sys.argv[2:] if len(sys.argv) > 2 else sys.argv[1:]
    if not args or args[0] == 'ls':
        r = sp.run(['rclone', 'lsl', REMOTE], capture_output=True, text=True)
        for l in r.stdout.strip().split('\n'): print(l)
        print(f'\n{len(r.stdout.strip().split(chr(10)))} files'); return
    if args[0] == 'all':
        r = sp.run(['rclone', 'lsf', REMOTE], capture_output=True, text=True)
        files = [f.strip() for f in r.stdout.strip().split('\n') if f.strip()]
        done = {os.path.splitext(f)[0] for f in os.listdir(OUT)} if os.path.isdir(OUT) else set()
        todo = [f for f in files if os.path.splitext(f)[0] not in done]
        print(f'{len(todo)}/{len(files)} to transcribe')
        for i, f in enumerate(todo):
            print(f'\n[{i+1}/{len(todo)}] {f}')
            lp = os.path.join(TMP, f)
            sp.run(['rclone', 'copy', f'{REMOTE}/{f}', TMP], check=True)
            txt, dur = transcribe(lp)
            if txt:
                out = os.path.join(OUT, os.path.splitext(f)[0] + '.txt')
                open(out, 'w').write(txt + '\n')
                print(f'  {dur:.0f}s -> {len(txt)} chars')
                print(f'  {txt[:200]}')
            else: print('  (empty)')
            os.remove(lp)
        return
    path = ' '.join(args)
    if os.path.isfile(path): lp = path; remote_dir = None
    else:
        lp = os.path.join(TMP, os.path.basename(path))
        remote_dir = path.rsplit('/', 1)[0] if ':' in path and '/' in path else REMOTE
        src = path if ':' in path else f'{REMOTE}/{path}'
        print(f'Downloading {src}...')
        sp.run(['rclone', 'copy', src, TMP], check=True)
    print(f'Transcribing {os.path.basename(lp)}...')
    txt, dur = transcribe(lp)
    if txt:
        base = os.path.splitext(os.path.basename(lp))[0]
        out = os.path.join(OUT, base + '.txt')
        open(out, 'w').write(txt + '\n')
        print(f'\n{dur:.0f}s | {len(txt)} chars | {out}')
        if remote_dir:
            sp.run(['rclone', 'copy', out, remote_dir], check=True)
            print(f'uploaded -> {remote_dir}/{base}.txt')
        print(txt)
    else: print('(no speech detected)')
    if remote_dir and lp.startswith(TMP):
        os.remove(lp); print(f'deleted local: {lp}')

run()
