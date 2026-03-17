"""a dash — top=live jobs, bottom=shell"""
import os,subprocess as sp
from _common import init_db,tm

def run():
    init_db();wd=os.getcwd()
    if not tm.has('dash'):
        sp.run(['tmux','new-session','-d','-s','dash','-c',wd,'watch','-n2','-c','a','jobs'])
        sp.run(['tmux','split-window','-v','-p','40','-t','dash','-c',wd])
    os.execvp('tmux',['tmux','switch-client'if'TMUX'in os.environ else'attach','-t','dash'])
