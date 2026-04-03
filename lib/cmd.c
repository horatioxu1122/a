/* cmd.c — shell passthrough for agents with direct shell access, 25us.
   claude code agents: redundant (bash tool adds same dispatch layer).
   direct shell (a j, scripts, cron, MCP, human): no dispatch, native speed. */
static int cmd_cmd(int c,char**v){if(c<3)return 1;
char cm[B]="";for(int i=2;i<c;i++)snprintf(cm+strlen(cm),(size_t)(B-strlen(cm)),"%s%s",i>2?" ":"",v[i]);
return system(cm);}
