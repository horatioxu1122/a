/* ── view: cross-platform visual interaction for LLM agents ──
   Priority order (agent should try each before falling back):
   1. Terminal/CLI command — fastest, most reliable, no screenshots
   2. Keyboard (a view key) — works everywhere
   3. UI Automation (a view uia) — WinUI/UWP/native, no coords needed
   4. Visual 9-grid fan (a view fan) — when you can only see it, ~2 rounds
   5. CDP JS eval — Chromium-specific, needs --remote-debugging-port=9222 */
static int view_wsl(void){char v[8];pcmd("grep -ci microsoft /proc/version 2>/dev/null",v,8);return atoi(v)>0;}

static void view_gridcalc(int x,int y,int w,int h,const char*pos,int zoom,int*nx,int*ny,int*nw,int*nh){
    int fx=50,fy=50;
    if(!strcmp(pos,"TL")){fx=25;fy=25;}else if(!strcmp(pos,"T")){fy=25;}else if(!strcmp(pos,"TR")){fx=75;fy=25;}
    else if(!strcmp(pos,"L")){fx=25;}else if(!strcmp(pos,"R")){fx=75;}
    else if(!strcmp(pos,"BL")){fx=25;fy=75;}else if(!strcmp(pos,"B")){fy=75;}else if(!strcmp(pos,"BR")){fx=75;fy=75;}
    *nw=w*zoom/100;*nh=h*zoom/100;if(*nw<60)*nw=60;if(*nh<60)*nh=60;
    *nx=x+w*fx/100-*nw/2;*ny=y+h*fy/100-*nh/2;
    if(*nx<0)*nx=0;if(*ny<0)*ny=0;}

/* write+run a .ps1 to avoid $ escaping hell */
static int view_ps(const char*script,char*out,int osz){
    writef("/mnt/c/tmp/v_cmd.ps1",script);
    return pcmd("powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:/tmp/v_cmd.ps1",out,osz);}

static void view_docrop(int wsl,const char*td,int x,int y,int w,int h,const char*out){
    char c[B];
    if(wsl){snprintf(c,B,
        "Add-Type -AssemblyName System.Drawing\n"
        "$b=[Drawing.Bitmap]::FromFile('C:/tmp/v_full.png')\n"
        "$c=$b.Clone((New-Object Drawing.Rectangle %d,%d,%d,%d),$b.PixelFormat)\n"
        "$g=[Drawing.Graphics]::FromImage($c)\n"
        "$g.FillEllipse([Drawing.Brushes]::Red,($c.Width/2-4),($c.Height/2-4),8,8)\n"
        "$g.Dispose();$c.Save('C:/tmp/%s');$b.Dispose();$c.Dispose()\n",x,y,w,h,out);
        view_ps(c,NULL,0);
#ifdef __APPLE__
    }else{snprintf(c,B,"convert %s/v_full.png -crop %dx%d+%d+%d "
        "-fill red -draw 'circle %d,%d %d,%d' %s/%s 2>/dev/null",td,w,h,x,y,w/2,h/2,w/2+4,h/2,td,out);
        system(c);
#else
    }else{snprintf(c,B,"convert %s/v_full.png -crop %dx%d+%d+%d "
        "-fill red -draw 'circle %d,%d %d,%d' %s/%s 2>/dev/null",td,w,h,x,y,w/2,h/2,w/2+4,h/2,td,out);
        system(c);
#endif
    }}

static int cmd_view(int argc,char**argv){
    AB;perf_disarm();
    const char*sub=argc>2?argv[2]:NULL;
    int wsl=view_wsl();const char*td=wsl?"/mnt/c/tmp":"/tmp";
    if(!sub){
        puts("a view — visual interaction for LLM agents\n\n"
            "  PRIORITY (try in order):\n"
            "  1. Terminal command — fastest. ex: a view open ms-settings:\n"
            "  2. a view key \"{ENTER}\" — keyboard, works everywhere\n"
            "  3. a view uia \"Button name\" — UI Automation (WinUI/native)\n"
            "  4. a view fan X Y W H [Z] — visual 9-grid search (~2 rounds)\n"
            "  5. CDP — Chromium JS eval (needs --remote-debugging-port)\n\n"
            "  COMMANDS:\n"
            "  shot              screenshot\n"
            "  fan X Y W H [Z]   9 parallel crops (Z=zoom%, default 40)\n"
            "  grid X Y W H P Z  single crop+dot (P=TL/T/TR/L/C/R/BL/B/BR)\n"
            "  click X Y         mouse click\n"
            "  key \"text\"        keystrokes (SendKeys/xdotool)\n"
            "  uia \"name\"        UI Automation click by name\n"
            "  open URI          open app/URI\n"
            "  crop X Y W H      manual crop with red dot");return 0;}

    if(!strcmp(sub,"shot")){char o[256];mkdirp((char*)td);
        if(wsl){view_ps(
            "Add-Type -AssemblyName System.Windows.Forms,System.Drawing\n"
            "$s=[Windows.Forms.Screen]::PrimaryScreen.Bounds\n"
            "$b=New-Object Drawing.Bitmap $s.Width,$s.Height\n"
            "[Drawing.Graphics]::FromImage($b).CopyFromScreen(0,0,0,0,$b.Size)\n"
            "$b.Save('C:/tmp/v_full.png');$b.Dispose()\n"
            "Write-Output \"$($s.Width) $($s.Height)\"\n",o,256);
            system("cp /mnt/c/tmp/v_full.png /mnt/c/tmp/v.png");
#ifdef __APPLE__
        }else{pcmd("screencapture -x /tmp/v_full.png;cp /tmp/v_full.png /tmp/v.png;"
            "sips -g pixelWidth -g pixelHeight /tmp/v_full.png 2>/dev/null|awk '/pixel/{printf $2\" \"}'",o,256);
#else
        }else{pcmd("scrot /tmp/v_full.png 2>/dev/null||grim /tmp/v_full.png 2>/dev/null;"
            "cp /tmp/v_full.png /tmp/v.png;identify -format '%w %h' /tmp/v_full.png 2>/dev/null",o,256);
#endif
        }printf("%s\n%s/v.png\n",o,td);return 0;}

    if(!strcmp(sub,"fan")&&argc>=7){
        int x=atoi(argv[3]),y=atoi(argv[4]),w=atoi(argv[5]),h=atoi(argv[6]),z=argc>7?atoi(argv[7]):40;
        const char*pos[]={"TL","T","TR","L","C","R","BL","B","BR"};
        for(int i=0;i<9;i++){int nx,ny,nw,nh;char f[32];
            view_gridcalc(x,y,w,h,pos[i],z,&nx,&ny,&nw,&nh);
            snprintf(f,32,"v_%s.png",pos[i]);view_docrop(wsl,td,nx,ny,nw,nh,f);}
        for(int i=0;i<9;i++){int nx,ny,nw,nh;
            view_gridcalc(x,y,w,h,pos[i],z,&nx,&ny,&nw,&nh);
            printf("%s: %d %d %d %d → %s/v_%s.png\n",pos[i],nx,ny,nw,nh,td,pos[i]);}
        return 0;}

    if(!strcmp(sub,"grid")&&argc>=8){
        int x=atoi(argv[3]),y=atoi(argv[4]),w=atoi(argv[5]),h=atoi(argv[6]),z=atoi(argv[8]);
        int nx,ny,nw,nh;view_gridcalc(x,y,w,h,argv[7],z,&nx,&ny,&nw,&nh);
        view_docrop(wsl,td,nx,ny,nw,nh,"v.png");
        printf("%d %d %d %d\n%s/v.png\n",nx,ny,nw,nh,td);return 0;}

    if(!strcmp(sub,"click")&&argc>=5){int x=atoi(argv[3]),y=atoi(argv[4]);char c[B];
        if(wsl){snprintf(c,B,
            "Add-Type -MemberDefinition '\n"
            "[DllImport(\"user32.dll\")]public static extern bool SetCursorPos(int x,int y);\n"
            "[DllImport(\"user32.dll\")]public static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr i);\n"
            "' -Name U -Namespace W\n"
            "[W.U]::SetCursorPos(%d,%d);Start-Sleep -m 150\n"
            "[W.U]::mouse_event(2,0,0,0,[UIntPtr]::Zero);Start-Sleep -m 50\n"
            "[W.U]::mouse_event(4,0,0,0,[UIntPtr]::Zero)\n",x,y);view_ps(c,NULL,0);
#ifdef __APPLE__
        }else{snprintf(c,B,"cliclick c:%d,%d 2>/dev/null",x,y);system(c);
#else
        }else{snprintf(c,B,"xdotool mousemove %d %d click 1 2>/dev/null||ydotool mousemove -a %d %d click 1",x,y,x,y);system(c);
#endif
        }printf("clicked (%d,%d)\n",x,y);return 0;}

    if(!strcmp(sub,"key")&&argc>=4){char k[512]="";
        for(int i=3;i<argc;i++){int l=(int)strlen(k);snprintf(k+l,512-l,"%s%s",l?" ":"",argv[i]);}
        if(wsl){char c[B];snprintf(c,B,
            "Add-Type -AssemblyName System.Windows.Forms\n"
            "[System.Windows.Forms.SendKeys]::SendWait('%s')\n",k);view_ps(c,NULL,0);
#ifdef __APPLE__
        }else{char c[B];snprintf(c,B,"osascript -e 'tell app \"System Events\" to keystroke \"%s\"'",k);system(c);
#else
        }else{char c[B];snprintf(c,B,"xdotool type -- '%s' 2>/dev/null||ydotool type -- '%s'",k,k);system(c);
#endif
        }printf("sent: %s\n",k);return 0;}

    if(!strcmp(sub,"uia")&&argc>=4){
        if(!wsl){puts("uia: Windows/WSL only");return 1;}
        char nm[512]="",c[B],o[64];
        for(int i=3;i<argc;i++){int l=(int)strlen(nm);snprintf(nm+l,512-l,"%s%s",l?" ":"",argv[i]);}
        snprintf(c,B,
            "Add-Type -AssemblyName UIAutomationClient,UIAutomationTypes\n"
            "$r=[System.Windows.Automation.AutomationElement]::RootElement\n"
            "$c=New-Object System.Windows.Automation.PropertyCondition("
            "[System.Windows.Automation.AutomationElement]::NameProperty,'%s')\n"
            "$b=$r.FindFirst([System.Windows.Automation.TreeScope]::Descendants,$c)\n"
            "if($b){$b.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke();'ok'}else{'not found'}\n",nm);
        view_ps(c,o,64);printf("%s\n",o);return 0;}

    if(!strcmp(sub,"open")&&argc>=4){char c[B];
#ifdef __APPLE__
        snprintf(c,B,"open '%s'",argv[3]);
#else
        if(wsl)snprintf(c,B,"powershell.exe -NoProfile -c \"Start-Process '%s'\"",argv[3]);
        else snprintf(c,B,"xdg-open '%s' 2>/dev/null",argv[3]);
#endif
        system(c);printf("opened: %s\n",argv[3]);return 0;}

    if(!strcmp(sub,"crop")&&argc>=7){
        int x=atoi(argv[3]),y=atoi(argv[4]),w=atoi(argv[5]),h=atoi(argv[6]);
        view_docrop(wsl,td,x,y,w,h,"v.png");
        printf("%d %d\n%s/v.png\n",x+w/2,y+h/2,td);return 0;}

    printf("unknown: a view %s\n",sub);return 1;}
