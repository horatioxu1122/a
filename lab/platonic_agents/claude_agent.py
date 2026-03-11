#!/usr/bin/env python3
import subprocess as S,anthropic,sys,select,readline;from pathlib import Path
E=dict(l.split('=',1)for l in(Path(__file__).resolve().parents[2]/'adata/git/login/api_keys.env').read_text().splitlines()if'='in l)
C,M,m,P=anthropic.Anthropic(api_key=E['ANTHROPIC_API_KEY']),(sys.argv[1:]+["claude-opus-4-6"])[0],[],"Linux CLI. ENTIRE reply: CMD:<cmd>. After output, text."
def rd():
    r=[input("\n> ")]
    while select.select([0],[],[],.05)[0]:r.append(input())
    return"\n".join(r)
while u:=rd().strip():
    m+=[{"role":"user","content":u}]
    for _ in"*****":
        t=C.messages.create(model=M,max_tokens=2048,system=P,messages=m[-20:]).content[0].text.strip();print(t);m+=[{"role":"assistant","content":t}];c=t[t.index("CMD:")+4:].split("\n")[0].strip()if"CMD:"in t else""
        if not c:break
        o=S.run(c,shell=1,capture_output=1,text=1,timeout=30).stdout.strip();print(f"${c}\n{o}");m+=[{"role":"user","content":f"<c>{c}</c>{o or'~'}"}]
