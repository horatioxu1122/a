#!/usr/bin/env python3
"""Platonic agent with delete-not-summarize memory. Numbered item list, LLM deletes least important to stay in budget."""
import subprocess as S,anthropic,sys,json;from pathlib import Path
K=dict(l.split('=',1)for l in(Path(__file__).resolve().parents[2]/'adata'/'git'/'login'/'api_keys.env').read_text().splitlines()if'='in l)
C=anthropic.Anthropic(api_key=K['ANTHROPIC_API_KEY']);M=sys.argv[1]if len(sys.argv)>1 else"claude-sonnet-4-6"
MF=Path(__file__).with_name("mem_agent.jsonl");MAX=40

def load():
    if not MF.exists():return[]
    return[json.loads(l)for l in MF.read_text().splitlines()if l.strip()]
def save(mem):MF.write_text("\n".join(json.dumps(m)for m in mem)+"\n"if mem else"")
def fmt(mem):return"\n".join(f"{i+1}. {m['t']}"for i,m in enumerate(mem))or"(empty)"

mem=load();msgs=[]
print(f"mem_agent | model={M} | {len(mem)} memories | MAX={MAX}")
while u:=input("\n> ").strip():
    msgs+=[{"role":"user","content":u}]
    for _ in range(5):
        sys_p=f"Linux CLI agent. CMD:<cmd> to run shell. MEM_ADD:<text> to remember. MEM_DEL:1,5,12 to delete by number. Delete aggressively — only keep what changes future decisions. Memory persists across restarts.\n\nMEMORY ({len(mem)}/{MAX}):\n{fmt(mem)}"
        r=C.messages.create(model=M,max_tokens=1024,system=sys_p,messages=msgs[-20:])
        t=r.content[0].text.strip();print(t)
        for line in t.split("\n"):
            if line.startswith("MEM_DEL:"):
                try:
                    ids=sorted(set(int(x)-1 for x in line[8:].split(",")if x.strip()),reverse=True)
                    for i in ids:
                        if 0<=i<len(mem):print(f"  [del {i+1}: {mem[i]['t'][:60]}]");mem.pop(i)
                    save(mem)
                except:pass
            elif line.startswith("MEM_ADD:"):
                if len(mem)>=MAX:print(f"  [auto-del 1: {mem[0]['t'][:60]}]");mem.pop(0)
                mem.append({"t":line[8:].strip()});save(mem);print(f"  [+mem {len(mem)}: {line[8:].strip()[:60]}]")
        msgs+=[{"role":"assistant","content":t}]
        c=t[t.index("CMD:")+4:].split("\n")[0].strip(" `")if"CMD:"in t else""
        if not c:break
        o=S.run(c,shell=1,capture_output=1,text=1,timeout=30).stdout.strip();print(f"${c}\n{o}");msgs+=[{"role":"user","content":f"`{c}`:\n{o or'~'}"}]
