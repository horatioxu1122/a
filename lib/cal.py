import sys,os,time,glob
from _common import SYNC_ROOT,DEVICE_ID

d=str(SYNC_ROOT/'cal');os.makedirs(d,exist_ok=True)
ts=time.strftime("%Y%m%dT%H%M%S")+f".{time.time_ns()%10**9:09d}_{DEVICE_ID}"
tf=f"{d}/{ts}.txt";s=sys.argv[2] if len(sys.argv)>2 else ""
if s=="add":
    if len(sys.argv)<4:print("missing event text");sys.exit(1)
    open(tf,"w").write(" ".join(sys.argv[3:])+"\n");print("added")
elif s=="ai":
    ex="".join(open(f).read() for f in sorted(glob.glob(f"{d}/*.txt")))
    pr=" ".join(sys.argv[3:]) if len(sys.argv)>3 else "Ask the user what they want to add or change in their calendar."
    td=time.strftime("%Y-%m-%d")
    os.execlp("a","a","c",f"Today is {td}. Calendar dir: {d}\nWrite to: {tf}\nAll events:\n{ex or '(empty)'}\n\nUser request: {pr}\n\nAdd events as lines: YYYY-MM-DD HH:MM description. Append to the write file.")
elif s:
    f=f"{d}/{s}" if "." in s else f"{d}/{s}.txt"
    open(f,"a").close();os.execlp("e","e",f)
else:
    evs=[ln.rstrip("\n") for f in sorted(glob.glob(f"{d}/*.txt")) for ln in open(f) if len(ln)>10 and ln[0]>='2' and ln[4]=='-' and ln[7]=='-']
    for e in sorted(evs):print(e)
    if not evs:print("no events")
    print(f"\n{d}/")
    for f in sorted(glob.glob(f"{d}/*.txt")):print(f"  {os.path.basename(f)}")
    print('\nusage:\n  a cal add "2026-03-06 09:00 standup"\n  a cal ai <prompt> ai-assisted add\n  a cal <name>      edit file')
