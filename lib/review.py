"""a review - Show GitHub PRs"""
import subprocess as sp,json
def run():
    items=json.loads(sp.run('gh search prs --state open --owner @me --json repository,number,title,url,author',shell=True,capture_output=True,text=True).stdout or '[]')
    [print(f"  {i}. {p['author']['login']} {p['repository']['nameWithOwner']}#{p['number']} {p['title']}") for i,p in enumerate(items)] or print("  (none)")
    if not items: return
    c=input("\n#/[c]lose all/q> ").strip()
    if not c or c[0] in 'q\x1b': return
    if c[0]=='c':sp.run(';'.join(f"gh pr close {p['url']}&" for p in items)+'wait',shell=True);print("✓");return
    if not c[0].isdigit(): return
    p=items[int(c)];repo,num=p['repository']['nameWithOwner'],str(p['number'])
    sp.run(f'D=/tmp/pr-{num};rm -rf $D&&gh repo clone {repo} $D>/dev/null 2>&1&&cd $D&&gh pr checkout {num}>/dev/null 2>&1',shell=True)
    sp.run(['tmux','new-window','-c',f'/tmp/pr-{num}','claude','--dangerously-skip-permissions',f"Review PR #{num} by {p['author']['login']} on {repo}: {p['title']}. Diff against main, review code, help merge to main. Only push on my approval."])
run()
