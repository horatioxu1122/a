"""a apk [path]"""
import os,subprocess as S,shutil,glob,sys
P="com.aios.a"
KT=r'''@file:Suppress("DEPRECATION","OVERRIDE_DEPRECATION")
package com.aios.a
import android.app.Activity;import android.content.*;import android.os.*;import android.webkit.*;import android.view.*;import android.graphics.*;import android.widget.*
import java.io.File
private const val U="http://127.0.0.1:1111/term"
class M:Activity(){
companion object{init{System.loadLibrary("anative")}}
private external fun nResize(w:Int,h:Int)
private external fun nRender(px:IntArray)
private external fun nFont(d:IntArray)
private lateinit var w:WebView;private val h=Handler(Looper.getMainLooper());private var n=0
private fun pg(s:String)=w.loadDataWithBaseURL(null,"<body style='font:18px monospace;padding:20px;background:#000;color:#0f0'>$s","text/html","utf-8",null)
@JavascriptInterface fun retry(){h.post{boot()}}
override fun onBackPressed(){if(w.canGoBack())w.goBack() else super.onBackPressed()}
override fun onResume(){super.onResume();boot()}
private val nl by lazy{applicationInfo.nativeLibraryDir}
private fun setup(){val ui=File(filesDir,"lib/ui");ui.mkdirs();val up=File(ui,"ui_full.py");if(!up.exists())assets.open("ui_full.py").use{i->up.outputStream().use{o->i.copyTo(o)}}
val ti=File(filesDir,"terminfo");if(!File(ti,"x/xterm-256color").exists()){ti.deleteRecursively();ti.mkdirs();val src=File(filesDir,"terminfo.src");if(!src.exists())assets.open("terminfo.src").use{i->src.outputStream().use{o->i.copyTo(o)}}
ProcessBuilder("$nl/libtic.so","-o",ti.absolutePath,src.absolutePath).redirectErrorStream(true).redirectOutput(File(filesDir,"tic.log")).start().waitFor()}}
private fun spawn(){val pb=ProcessBuilder("$nl/liba.so","serve","1111");pb.environment().apply{put("PATH","$nl:/system/bin");put("TMUX_BIN","$nl/libtmux.so");put("TIC_BIN","$nl/libtic.so");put("TMUX_TMPDIR",filesDir.absolutePath);put("TERMINFO","${filesDir}/terminfo");put("HOME",filesDir.absolutePath);put("A_SDIR",filesDir.absolutePath)};pb.redirectErrorStream(true);pb.redirectOutput(File(filesDir,"serve.log"));try{pb.start()}catch(x:Exception){pg("spawn failed: $x")}}
private fun boot(){setup();spawn();n=0;h.postDelayed({w.loadUrl(U)},1800)}
private fun atlas(sz:Float):IntArray{val p=Paint().apply{textSize=sz;color=-1;isAntiAlias=true;typeface=Typeface.MONOSPACE};val cw=p.measureText("M").toInt()+1;val ch=(-p.ascent()+p.descent()).toInt()+1;val b=Bitmap.createBitmap(cw*95,ch,Bitmap.Config.ARGB_8888);Canvas(b).let{c->for(i in 0 until 95)c.drawText(((32+i).toChar()).toString(),(i*cw).toFloat(),-p.ascent(),p)};val r=IntArray(2+cw*95*ch);r[0]=cw;r[1]=ch;b.getPixels(r,2,cw*95,0,0,cw*95,ch);b.recycle();return r}
@android.annotation.SuppressLint("ClickableViewAccessibility")
override fun onCreate(b:Bundle?){super.onCreate(b)
WebView.setWebContentsDebuggingEnabled(true)
w=WebView(this).apply{settings.javaScriptEnabled=true;addJavascriptInterface(this@M,"A")
webChromeClient=object:WebChromeClient(){override fun onConsoleMessage(m:ConsoleMessage):Boolean{android.util.Log.w("AWV","[${m.messageLevel()}] ${m.message()} @ ${m.sourceId()}:${m.lineNumber()}");return true}}
webViewClient=object:WebViewClient(){override fun onReceivedError(v:WebView,r:WebResourceRequest,e:WebResourceError){if(r.isForMainFrame){if(n++<8){pg("<h2>Starting a serve...</h2>$n/8");h.postDelayed({v.loadUrl(U)},1500)}else pg("<h2>a serve not reachable</h2><button onclick='A.retry()'>Retry</button><br><br>log: $filesDir/serve.log")}}}}
val nv=object:View(this){private lateinit var bm:Bitmap;private var px:IntArray?=null
override fun onSizeChanged(w:Int,h:Int,ow:Int,oh:Int){if(::bm.isInitialized)bm.recycle();bm=Bitmap.createBitmap(w,h,Bitmap.Config.ARGB_8888);px=IntArray(w*h);nResize(w,h);nFont(atlas(48f))}
override fun onDraw(c:Canvas){val a=px?:return;nRender(a);bm.setPixels(a,0,width,0,0,width,height);c.drawBitmap(bm,0f,0f,null)}}
nv.visibility=View.GONE
val fr=FrameLayout(this);fr.addView(w);fr.addView(nv)
fun tab(s:String)=TextView(this).apply{text=s;gravity=Gravity.CENTER;setTextColor(-1);setPadding(0,32,0,32);layoutParams=LinearLayout.LayoutParams(0,-2,1f)}
val bar=LinearLayout(this).apply{orientation=LinearLayout.HORIZONTAL;setBackgroundColor(0xFF222222.toInt())}
tab("Web").also{it.setOnTouchListener{_,e->if(e.action==MotionEvent.ACTION_DOWN){w.visibility=View.VISIBLE;nv.visibility=View.GONE};false};bar.addView(it)}
tab("Native").also{it.setOnTouchListener{_,e->if(e.action==MotionEvent.ACTION_DOWN){w.visibility=View.GONE;nv.visibility=View.VISIBLE;nv.invalidate()};false};bar.addView(it)}
val root=LinearLayout(this).apply{orientation=LinearLayout.VERTICAL}
root.addView(fr,LinearLayout.LayoutParams(-1,0,1f));root.addView(bar);setContentView(root)}}
'''
NC=r'''#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
static int W,H;
typedef struct{uint32_t*px;int cw,ch,aw;}Font;
static Font F;
JNIEXPORT void JNICALL Java_com_aios_a_M_nResize(JNIEnv*e,jobject o,jint w,jint h){(void)o;W=w;H=h;}
JNIEXPORT void JNICALL Java_com_aios_a_M_nFont(JNIEnv*e,jobject o,jintArray d){(void)o;
jint*p=(*e)->GetIntArrayElements(e,d,0);F.cw=p[0];F.ch=p[1];int sz=F.cw*95*F.ch;
free(F.px);F.px=malloc((size_t)sz*4);memcpy(F.px,p+2,(size_t)sz*4);F.aw=F.cw*95;
(*e)->ReleaseIntArrayElements(e,d,p,0);}
static void dch(uint32_t*p,int x,int y,char c){if(c<32||c>126||!F.px)return;int ox=(c-32)*F.cw;
for(int j=0;j<F.ch;j++)for(int i=0;i<F.cw;i++){int px_=x+i,py=y+j;
if(px_>=0&&px_<W&&py>=0&&py<H){uint32_t s=F.px[j*F.aw+ox+i];if(s&0xFF000000)p[py*W+px_]=s;}}}
static void dt(uint32_t*p,int x,int y,const char*t){for(;*t;t++,x+=F.cw)dch(p,x,y,*t);}
JNIEXPORT void JNICALL Java_com_aios_a_M_nRender(JNIEnv*e,jobject o,jintArray px){(void)o;
jint*p=(*e)->GetIntArrayElements(e,px,0);for(int i=0;i<W*H;i++)p[i]=0xFF111111;
if(F.px){int tw=(int)strlen("a")*F.cw;dt(p,(W-tw)/2,H/3,"a");
tw=(int)strlen("agent manager")*F.cw;dt(p,(W-tw)/2,H/3+F.ch+20,"agent manager");}
(*e)->ReleaseIntArrayElements(e,px,p,0);}
'''
CML='cmake_minimum_required(VERSION 3.22)\nproject(anative)\nadd_library(anative SHARED native.c)\ntarget_compile_options(anative PRIVATE -O3 -flto)\ntarget_link_options(anative PRIVATE -flto)\ntarget_link_libraries(anative log)\n'
MF='<manifest xmlns:android="http://schemas.android.com/apk/res/android"><uses-permission android:name="android.permission.INTERNET"/><application android:usesCleartextTraffic="true" android:extractNativeLibs="true" android:networkSecurityConfig="@xml/nsc" android:label="a apk"><activity android:name=".M" android:exported="true"><intent-filter><action android:name="android.intent.action.MAIN"/><category android:name="android.intent.category.LAUNCHER"/></intent-filter></activity></application></manifest>'
NSC='<?xml version="1.0" encoding="utf-8"?><network-security-config><base-config cleartextTrafficPermitted="true"><trust-anchors><certificates src="system"/></trust-anchors></base-config><domain-config cleartextTrafficPermitted="true"><domain includeSubdomains="true">127.0.0.1</domain><domain includeSubdomains="true">localhost</domain></domain-config></network-security-config>'
GS='pluginManagement{repositories{google();mavenCentral()};plugins{id("com.android.application") version "8.2.0";id("org.jetbrains.kotlin.android") version "1.9.22"}}\ndependencyResolutionManagement{repositories{google();mavenCentral()}}\ninclude(":app")\n'
H=os.path.expanduser("~");IT=os.path.exists("/data/data/com.termux")
_CMK='externalNativeBuild{cmake{path=file("src/main/cpp/CMakeLists.txt")}}\n'
_DF='defaultConfig{applicationId="'+P+'";minSdk=24;targetSdk=34;versionCode=202;ndk{abiFilters+="arm64-v8a"}'+((';externalNativeBuild{cmake{arguments+="-DANDROID_STL=none"}}') if not IT else '')+'}\n'
GB='plugins{id("com.android.application");id("org.jetbrains.kotlin.android")}\nandroid{namespace="'+P+'";compileSdk=34;'+_DF+('' if IT else _CMK)+'compileOptions{sourceCompatibility=JavaVersion.VERSION_11;targetCompatibility=JavaVersion.VERSION_11}\nkotlinOptions{jvmTarget="11"}}\n'
SDK="/data/data/com.termux/files/home/android-sdk" if IT else os.environ.get("ANDROID_HOME",H+"/Android/Sdk")
R=os.path.dirname(os.path.dirname(os.path.abspath(__file__)));D=R+"/adata/_apk_build"
if not IT:
    for v in ["21","17"]:
        p=f"/usr/lib/jvm/java-{v}-openjdk-amd64"
        if os.path.exists(p):os.environ["JAVA_HOME"]=p;break
def w(p,s):os.makedirs(os.path.dirname(p),exist_ok=True);open(p,"w").write(s)
def adb(*a,serial=None):return S.run(["adb"]+(["-s",serial] if serial else[])+list(a),capture_output=True,text=True)
def devlist():return[l.split('\t')[0] for l in adb("devices").stdout.strip().split('\n')[1:] if '\tdevice' in l]
def _self_adb():
    """On Termux, try wireless adb to self-install. Returns serial or None."""
    if not IT:return None
    adb("connect","localhost:5555")
    out=adb("devices").stdout
    for l in out.strip().split('\n')[1:]:
        p=l.split('\t')
        if len(p)<2:continue
        s,st=p[0],p[1]
        if not any(x in s for x in["localhost","127.0.0.1","emulator"]):continue
        if st=="device":return s
        if st=="unauthorized":print("! ADB connected but unauthorized. Tap 'Allow' on the USB debugging dialog, then retry.");return None
    return None
def pick(ds):
    if len(ds)==1:return ds[0]
    for i,d in enumerate(ds):print(f"  {i}: {adb('-s',d,'shell','getprop','ro.product.model').stdout.strip() or d} ({d})")
    return ds[int(input("#: "))]
CP={0xd03:"a53",0xd05:"a55",0xd0b:"a76",0xd0d:"a77",0xd41:"a78",0xd44:"x1",0xd46:"a510",0xd47:"a710",0xd48:"x2",0xd4d:"a715",0xd4e:"x3",0xd80:"a520",0xd81:"a720",0xd82:"x4"}
def detect_cpu(serial):
    best=None
    for l in adb("shell","cat","/proc/cpuinfo",serial=serial).stdout.splitlines():
        if "CPU part" in l:
            c=CP.get(int(l.split(":")[-1].strip(),16))
            if c:best="cortex-"+c
    if best:print("cpu:",best)
    return best
def run():
    proj=serial=None
    for a in sys.argv[2:]:
        for p in [a,H+"/"+a,R+"/adata/git/my/"+a]:
            if os.path.isdir(p) and glob.glob(p+"/build.gradle*"):proj=os.path.abspath(p);break
        if proj:break
        elif not serial:serial=a
    if proj:
        if not os.path.exists(proj+"/gradlew"):sys.exit("x No gradlew in "+proj)
        lp=proj+"/local.properties"
        if not os.path.exists(lp) or SDK not in open(lp).read():open(lp,"w").write(f"sdk.dir={SDK}\n")
        if not serial:
            ds=devlist()
            if ds:serial=ds[0] if len(ds)==1 else pick(ds)
        cpu=detect_cpu(serial) if serial else None
        ga=["./gradlew","assembleDebug"]
        if cpu:ga.append(f"-Pcpu_target={cpu}")
        os.chdir(proj);S.run(ga,check=True)
        apks=glob.glob(proj+"/**/debug/*.apk",recursive=True)
        if not apks:sys.exit("x No APK")
        apk=apks[0];pkg=None
        for bf in glob.glob(proj+"/app/build.gradle*"):
            for line in open(bf):
                if ("applicationId" in line or "namespace" in line) and '"' in line:pkg=line.split('"')[1];break
    else:
        w(D+"/settings.gradle.kts",GS);w(D+"/app/build.gradle.kts",GB);w(D+"/local.properties",f"sdk.dir={SDK}\n")
        gp="android.useAndroidX=true\norg.gradle.jvmargs=-Xmx4g\n"
        if IT:gp+="android.aapt2FromMavenOverride=/data/data/com.termux/files/usr/bin/aapt2\n"
        w(D+"/gradle.properties",gp);w(D+"/app/src/main/AndroidManifest.xml",MF);w(D+"/app/src/main/java/com/aios/a/M.kt",KT);w(D+"/app/src/main/res/xml/nsc.xml",NSC)
        if IT:
            sf=D+"/app/src/main/jniLibs/arm64-v8a";os.makedirs(sf,exist_ok=True)
            w(D+"/native.c",NC);so=sf+"/libanative.so"
            S.run(f"clang -shared -O3 -flto -w -o '{so}' '{D}/native.c'&&patchelf --remove-rpath '{so}'",shell=True,check=True)
        else:w(D+"/app/src/main/cpp/native.c",NC);w(D+"/app/src/main/cpp/CMakeLists.txt",CML)
        # Stage bundled bins + terminfo source (from a droid/droidtmux output)
        sf=D+"/app/src/main/jniLibs/arm64-v8a";os.makedirs(sf,exist_ok=True)
        ad=D+"/app/src/main/assets";os.makedirs(ad,exist_ok=True)
        stage={"/tmp/a-droid":"liba.so","/tmp/dsrc/tmux-build-a/tmux":"libtmux.so","/tmp/dsrc/ncurses-6.4/progs/tic":"libtic.so"}
        miss=[s for s in stage if not os.path.exists(s)]
        if miss:sys.exit(f"x run 'a droid && a droidtmux' first: missing {miss}")
        for s,n in stage.items():shutil.copy(s,f"{sf}/{n}")
        shutil.copy("/tmp/dsrc/ncurses-6.4/misc/terminfo.src",f"{ad}/terminfo.src")
        shutil.copy(R+"/lib/ui/ui_full.py",f"{ad}/ui_full.py")
        if not os.path.exists(D+"/gradlew"):
            for s in glob.glob(H+"/*/gradlew")+glob.glob(R+"/adata/git/my/*/gradlew"):
                d=os.path.dirname(s);shutil.copy(s,D+"/gradlew");os.chmod(D+"/gradlew",0o755)
                wd=d+"/gradle/wrapper";os.makedirs(D+"/gradle/wrapper",exist_ok=True)
                for f in os.listdir(wd):shutil.copy(wd+"/"+f,D+"/gradle/wrapper/")
                break
            else:sys.exit("x No gradlew")
        os.chdir(D);S.run(["./gradlew","--no-configuration-cache","assembleDebug"],check=True)
        apk=D+"/app/build/outputs/apk/debug/app-debug.apk";pkg=P
    if IT:
        sa=_self_adb()
        if sa:
            r=adb("install","-r",apk,serial=sa)
            if "INSTALL_FAILED" in r.stdout+r.stderr:
                if pkg:adb("uninstall",pkg,serial=sa)
                r=adb("install",apk,serial=sa)
            if r.returncode==0:
                if pkg:adb("shell","am","start","-n",pkg+"/.M",serial=sa)
                print("✓ "+(pkg or os.path.basename(apk)));return
            print("x adb install failed, falling back to manual")
        S.run(["cp",apk,"/storage/emulated/0/Download/"+os.path.basename(apk)],check=True)
        if not sa:print("! Enable wireless debug for auto-install:\n  Settings → Developer Options → Wireless debugging ON\n  adb connect localhost:5555  → tap Allow\n  APK copied to Downloads")
        if pkg:S.run(["am","start","-n",pkg+"/.M"])
    else:
        if not serial:
            ds=devlist()
            if not ds:sys.exit("No devices")
            serial=pick(ds)
        r=adb("install","-r",apk,serial=serial)
        if "INSTALL_FAILED" in r.stdout+r.stderr:
            if pkg:adb("uninstall",pkg,serial=serial)
            r=adb("install",apk,serial=serial)
        if r.returncode:print(r.stderr);sys.exit(1)
        if pkg:adb("shell","am","start","-n",pkg+"/.M",serial=serial)
    print("✓ "+(pkg or os.path.basename(apk)))
run()
