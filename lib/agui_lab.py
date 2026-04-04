#!/usr/bin/env python3
# experimental
"""lab.py — 0-cost agent experiments via web LLM browser scraping.
CMD: protocol. Platonic style. Browser subscription = free inference.

  python3 lab.py agent [platform]     interactive CMD: agent ($0/query)
  python3 lab.py stream [platform]    real-time DOM token streaming
  python3 lab.py code [platform]      ollama writes + web reviews, alternate
"""
import sys,os,time,json,subprocess as S
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import agui

P={# name: (url, [input_sels], response_sel)
'claude':('https://claude.ai/',['div[contenteditable="true"]'],'.font-claude-response'),
'chatgpt':('https://chatgpt.com/',['div[contenteditable="true"]','#prompt-textarea'],'div[data-message-author-role="assistant"] .markdown'),
'grok':('https://grok.com/',['div[contenteditable="true"]','textarea'],'.message-text'),
'deepseek':('https://chat.deepseek.com/',['textarea'],'.ds-markdown'),
'gemini':('https://gemini.google.com/app',['div[role="textbox"]'],'message-content .markdown'),
}

def _pg():
    p=agui.get_page()
    if p:return p
    agui.launch_browser_with_positioning();return agui.get_page()

def _nav(nm):_pg().goto(P[nm][0],wait_until='domcontentloaded',timeout=30000);time.sleep(3)

def _send(nm,txt):
    pg=_pg()
    for sel in P[nm][1]:
        try:
            pg.wait_for_selector(sel,timeout=8000,state='visible')
            el=pg.locator(sel).first;el.click(timeout=3000);time.sleep(0.3)
            try:el.fill(txt,timeout=2000)
            except:pg.keyboard.type(txt,delay=5)
            pg.keyboard.press('Enter');return True
        except:continue
    return False

def _nresp(nm):
    try:return len(_pg().query_selector_all(P[nm][2]))
    except:return 0

def _last(nm):
    try:els=_pg().query_selector_all(P[nm][2]);return els[-1].inner_text()if els else""
    except:return""

def web(nm,txt,new=False):
    """Full response from web LLM. $0."""
    if new:_nav(nm)
    n0=_nresp(nm)
    if not _send(nm,txt):return"[ERR:input]"
    prev="";hits=[]
    for _ in range(600):
        time.sleep(0.2)
        if _nresp(nm)<=n0:
            if hits and _stream_done(hits):break
            continue
        t=_last(nm)
        if t!=prev and len(t)>len(prev):hits.append(time.time());prev=t
        elif hits and _stream_done(hits):break
    return _last(nm)or"[TIMEOUT]"

def _stream_done(hits,abs_max=3.0):
    """Detect end-of-stream from token arrival cadence.
    Done when gap > 5x avg interval (adaptive) or >abs_max (hard cap)."""
    if not hits:return False
    gap=time.time()-hits[-1]
    if gap>abs_max:return True
    if len(hits)<2:return False
    iv=[hits[i]-hits[i-1]for i in range(1,len(hits))]
    avg=sum(iv)/len(iv)
    return gap>max(avg*5,1.0)

def stream(nm,txt,new=False):
    """Stream response tokens to stdout. Auto-detects end via token cadence. $0."""
    if new:_nav(nm)
    n0=_nresp(nm)
    if not _send(nm,txt):print("[ERR:input]");return""
    prev="";hits=[]
    for _ in range(3000):  # 5 min at 100ms
        time.sleep(0.1)
        if _nresp(nm)<=n0:
            if hits and _stream_done(hits):break
            continue
        t=_last(nm)
        if len(t)>len(prev):
            now=time.time();hits.append(now)
            print(t[len(prev):],end='',flush=True);prev=t
        elif hits and _stream_done(hits):break
    if hits and len(hits)>1:
        iv=[hits[i]-hits[i-1]for i in range(1,len(hits))]
        print(f"\n[{len(hits)} chunks, avg {sum(iv)/len(iv)*1000:.0f}ms/chunk, {len(prev)} chars]")
    else:print()
    return prev

def local(p,model='devstral-small-2:24b'):
    """Ollama. $0."""
    from urllib.request import urlopen,Request
    return json.loads(urlopen(Request('http://localhost:11434/api/generate',
        json.dumps({"model":model,"prompt":p,"stream":False}).encode(),
        {"Content-Type":"application/json"}),timeout=300).read()).get('response','')

def _code(t):
    if'```'in t:
        p=t.split('```')
        if len(p)>2:
            b=p[1]
            if b.startswith('python\n'):b=b[7:]
            elif b.startswith('python3\n'):b=b[8:]
            elif b.startswith('\n'):b=b[1:]
            return b.rstrip()
    return t.strip()

# ═══ experiments ═══

def exp_agent(nm='claude'):
    """CMD: agent loop via web browser. $0/query."""
    _nav(nm);first=True
    while u:=input("\n> ").strip():
        for _ in range(3):
            msg=("CLI agent. CMD:<cmd> or text.\n"+u)if first else u;first=False
            t=stream(nm,msg)
            c=t[t.index("CMD:")+4:].split("\n")[0].strip(" `")if"CMD:"in t else""
            if not c:break
            o=S.run(c,shell=True,capture_output=True,text=True,timeout=30).stdout.strip()
            print(f"${c}\n{o}");u=f"`{c}`:\n{o or'~'}"

def exp_stream(nm='claude'):
    """Real-time streaming from one web model."""
    _nav(nm)
    while u:=input("\n> ").strip():stream(nm,u)

def exp_code(nm='claude',lm='devstral-small-2:24b'):
    """Ollama writes → run → web reviews → fix → repeat. Both $0."""
    _nav(nm);task=input("task> ")
    print(f"[local:{lm}] [web:{nm}]")
    code=_code(local(f"Write Python: {task}\nOnly code, no markdown.",lm))
    for i in range(5):
        print(f"\n{'='*40} round {i+1}\n{code[:1000]}")
        with open('/tmp/_lab.py','w')as f:f.write(code)
        r=S.run(['python3','/tmp/_lab.py'],capture_output=True,text=True,timeout=30)
        if r.returncode==0:print(f"✓\n{r.stdout[:300]}");return
        err=r.stderr[:400];print(f"✗\n{err}")
        if i%2==0:raw=web(nm,f"Fix this Python. ONLY code.\n```python\n{code}\n```\nError:{err}",new=True)
        else:raw=local(f"Fix this Python. ONLY code.\n{code}\nError:{err}",lm)
        code=_code(raw)
    print("✗ exhausted")

if __name__=='__main__':
    a=sys.argv[1:];cmd=a[0]if a else'agent';nm=a[1]if len(a)>1 else'claude'
    try:{'agent':exp_agent,'stream':exp_stream,'code':exp_code}[cmd](nm)
    except(KeyboardInterrupt,EOFError):print("\n✓")
    finally:agui.close_browser()
