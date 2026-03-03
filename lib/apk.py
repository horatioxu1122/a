"""a apk - Build+install A android app (Termux bridge + WebView)"""
import os,subprocess as S,shutil,glob,sys

PKG="com.aios.a"
KT=r'''package com.aios.a
import android.app.Activity
import android.content.*
import android.content.pm.PackageManager
import android.os.*
import android.webkit.*

private const val URL="http://localhost:1111"
private fun pg(t:String,b:String)="<body style='font:18px monospace;padding:40px;background:#1a1a1a;color:#0f0'><h2>$t</h2><p style='color:#ccc'>$b</p>"

class MainActivity:Activity(){
    private lateinit var wv:WebView
    private val h=Handler(Looper.getMainLooper())
    private var tries=0;private var booted=false

    private fun runCmd(){try{startForegroundService(Intent().apply{
        setClassName("com.termux","com.termux.app.RunCommandService")
        action="com.termux.RUN_COMMAND"
        putExtra("com.termux.RUN_COMMAND_PATH","/data/data/com.termux/files/usr/bin/bash")
        putExtra("com.termux.RUN_COMMAND_ARGUMENTS",arrayOf("-l","-c","a ui on"))
        putExtra("com.termux.RUN_COMMAND_BACKGROUND",true)
    })}catch(_:Exception){}}

    private fun show(html:String)=wv.loadDataWithBaseURL(null,html,"text/html","utf-8",null)

    inner class Bridge{
        @JavascriptInterface fun copy(){(getSystemService(CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("","a ui on"))}
        @JavascriptInterface fun openTermux(){try{startActivity(Intent().apply{
            setClassName("com.termux","com.termux.app.TermuxActivity");addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        })}catch(_:Exception){}}
        @JavascriptInterface fun retry(){h.post{booted=false;boot()}}
    }

    private fun start():String?{
        if(checkSelfPermission("com.termux.permission.RUN_COMMAND")!=PackageManager.PERMISSION_GRANTED)
            return pg("Permission needed","1. Settings → Apps → <b>a apk</b> → Permissions<br>→ enable <b>Run commands in Termux</b><br><br>2. In Termux run:<br><code style='color:#ff0'>echo 'allow-external-apps=true' >> ~/.termux/termux.properties</code><br><br>Then reopen this app")
        runCmd();return null
    }

    @Suppress("DEPRECATION") override fun onBackPressed(){if(wv.canGoBack())wv.goBack() else super.onBackPressed()}
    override fun onResume(){super.onResume();if(!booted)boot()}

    private fun boot(){val e=start();if(e!=null){show(e);return};booted=true;tries=0;wv.loadUrl(URL)}

    override fun onCreate(b:Bundle?){super.onCreate(b)
        wv=WebView(this).apply{settings.javaScriptEnabled=true;addJavascriptInterface(Bridge(),"A")
            webViewClient=object:WebViewClient(){@Suppress("DEPRECATION")
                override fun onReceivedError(v:WebView,c:Int,d:String?,u:String?){
                    if(tries++<10){show(pg("Starting a ui...","attempt $tries/10"));if(tries%3==0)runCmd();h.postDelayed({v.loadUrl(URL)},2000)
                    }else show(pg("Server not responding","<style>button{font:18px monospace;padding:12px 24px;margin:8px 8px 8px 0;border:2px solid #0f0;background:#222;color:#0f0;border-radius:8px}</style><button onclick='A.copy()'>Copy: a ui on</button><button onclick='A.openTermux()'>Open Termux</button><br><br><button onclick='A.retry()'>Retry</button>"))
                }}}
        setContentView(wv)}
}'''
MANIFEST=f'''<manifest xmlns:android="http://schemas.android.com/apk/res/android">
<uses-permission android:name="android.permission.INTERNET"/>
<uses-permission android:name="com.termux.permission.RUN_COMMAND"/>
<application android:usesCleartextTraffic="true" android:label="a apk">
<activity android:name=".MainActivity" android:exported="true">
<intent-filter><action android:name="android.intent.action.MAIN"/><category android:name="android.intent.category.LAUNCHER"/></intent-filter>
</activity></application></manifest>'''
SETTINGS='pluginManagement{repositories{google();mavenCentral();gradlePluginPortal()};plugins{id("com.android.application") version "8.2.0";id("org.jetbrains.kotlin.android") version "1.9.22"}}\ndependencyResolutionManagement{repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS);repositories{google();mavenCentral()}}\nrootProject.name="A"\ninclude(":app")\n'
BUILD=f'plugins{{id("com.android.application");id("org.jetbrains.kotlin.android")}}\nandroid{{namespace="{PKG}";compileSdk=34;defaultConfig{{applicationId="{PKG}";minSdk=24;targetSdk=34;versionCode=200}}\ncompileOptions{{sourceCompatibility=JavaVersion.VERSION_11;targetCompatibility=JavaVersion.VERSION_11}}\nkotlinOptions{{jvmTarget="11"}}}}\n'

H=os.path.expanduser("~")
IS_T=os.path.exists("/data/data/com.termux")
SDK="/data/data/com.termux/files/home/android-sdk" if IS_T else os.environ.get("ANDROID_HOME",H+"/Android/Sdk")
SRC=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
D=os.path.join(SRC,"adata","_apk_build")
if not IS_T:
    for v in ["21","17"]:
        p=f"/usr/lib/jvm/java-{v}-openjdk-amd64"
        if os.path.exists(p):os.environ["JAVA_HOME"]=p;break

def w(p,s):os.makedirs(os.path.dirname(p),exist_ok=True);open(p,"w").write(s)

def adb(*a,serial=None):
    return S.run(["adb"]+(["-s",serial] if serial else [])+list(a),capture_output=True,text=True)

def run():
    w(f"{D}/settings.gradle.kts",SETTINGS)
    w(f"{D}/app/build.gradle.kts",BUILD)
    w(f"{D}/local.properties",f"sdk.dir={SDK}\n")
    gp="android.useAndroidX=true\norg.gradle.jvmargs=-Xmx4g\n"
    if IS_T:gp+="android.aapt2FromMavenOverride=/data/data/com.termux/files/usr/bin/aapt2\n"
    w(f"{D}/gradle.properties",gp)
    w(f"{D}/app/src/main/AndroidManifest.xml",MANIFEST)
    w(f"{D}/app/src/main/java/com/aios/a/MainActivity.kt",KT)
    if not os.path.exists(f"{D}/gradlew"):
        for src in [f"{SRC}/lab/android/A",*glob.glob(H+"/projects/androidDev/apks/*")]:
            gw=os.path.join(src,"gradlew")
            if os.path.exists(gw):
                shutil.copy(gw,f"{D}/gradlew");os.chmod(f"{D}/gradlew",0o755)
                wd=os.path.join(src,"gradle/wrapper")
                os.makedirs(f"{D}/gradle/wrapper",exist_ok=True)
                for f in os.listdir(wd):shutil.copy(os.path.join(wd,f),f"{D}/gradle/wrapper/")
                break
        else:sys.exit("x No gradlew found")
    os.chdir(D)
    S.run(["./gradlew","--no-configuration-cache","assembleDebug"],check=True)
    apk="app/build/outputs/apk/debug/app-debug.apk"
    if IS_T:
        dst="/storage/emulated/0/Download/a.apk"
        S.run(["cp",apk,dst],check=True)
        S.run(["am","start","-n","com.example.installer/.MainActivity","--es","apk_path",dst])
    else:
        serial=sys.argv[2] if len(sys.argv)>2 and sys.argv[2]!="apk" else None
        if not serial:
            devs=[l.split('\t')[0] for l in adb("devices").stdout.strip().split('\n')[1:] if '\tdevice' in l]
            if not devs:sys.exit("No devices")
            if len(devs)==1:serial=devs[0]
            else:
                for i,d in enumerate(devs):
                    name=adb("-s",d,"shell","getprop","ro.product.model").stdout.strip() or d
                    print(f"  {i}: {name} ({d})")
                serial=devs[int(input("#: "))]
        r=adb("install","-r",apk,serial=serial)
        out=r.stdout+r.stderr
        if "INSTALL_FAILED_UPDATE_INCOMPATIBLE" in out or "INSTALL_FAILED_VERSION_DOWNGRADE" in out:
            print("Signature mismatch, reinstalling...")
            adb("uninstall",PKG,serial=serial);r=adb("install",apk,serial=serial)
        if r.returncode:print(r.stderr);sys.exit(1)
        adb("shell","am","start","-n",f"{PKG}/.MainActivity",serial=serial)
    print(f"✓ {PKG}")
run()
