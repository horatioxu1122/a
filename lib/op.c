/* op — claude operator in tmux. plain english → a cmds. reattach per dir. */
static int cmd_op(int c,char**v){(void)c;(void)v;perf_disarm();
    init_db();load_cfg();CWD(wd);
    char cmd[B];snprintf(cmd,B,"claude --dangerously-skip-permissions --effort max --append-system-prompt-file %s/common/prompts/operator.txt",SROOT);
    char sn[64];snprintf(sn,64,"op-%s",bname(wd));
    if(!tm_has(sn))create_sess(sn,wd,cmd,NULL);
    tm_go(sn);return 0;}
