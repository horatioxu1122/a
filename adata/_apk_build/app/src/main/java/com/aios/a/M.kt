@file:Suppress("DEPRECATION","OVERRIDE_DEPRECATION")
package com.aios.a
import android.app.Activity;import android.content.*;import android.os.*;import android.webkit.*;import android.view.*;import android.graphics.*;import android.widget.*
private const val U="http://localhost:1111";private const val T="com.termux"
class M:Activity(){
companion object{init{System.loadLibrary("anative")}}
private external fun nResize(w:Int,h:Int)
private external fun nRender(px:IntArray)
private external fun nFont(d:IntArray)
private lateinit var w:WebView;private val h=Handler(Looper.getMainLooper());private var n=0
private fun tx(){try{startForegroundService(Intent().apply{setClassName(T,"$T.app.RunCommandService");action="$T.RUN_COMMAND";putExtra("$T.RUN_COMMAND_PATH","/data/data/$T/files/usr/bin/bash");putExtra("$T.RUN_COMMAND_ARGUMENTS",arrayOf("-l","-c","a ui on"));putExtra("$T.RUN_COMMAND_BACKGROUND",true)})}catch(_:Exception){}}
private fun pg(s:String)=w.loadDataWithBaseURL(null,"<body style='font:18px monospace;padding:20px;background:#000;color:#0f0'>$s","text/html","utf-8",null)
@JavascriptInterface fun copy(){(getSystemService(CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("","a ui on"))}
@JavascriptInterface fun termux(){try{startActivity(Intent().apply{setClassName(T,"$T.app.TermuxActivity");addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)})}catch(_:Exception){}}
@JavascriptInterface fun retry(){h.post{boot()}}
override fun onBackPressed(){if(w.canGoBack())w.goBack() else super.onBackPressed()}
override fun onResume(){super.onResume();boot()}
private fun boot(){if(checkSelfPermission("$T.permission.RUN_COMMAND")!=0){pg("<h2>Permission needed</h2>Settings→Apps→a→enable Run commands in Termux<br><br>In Termux:<br><code style='color:#ff0'>echo 'allow-external-apps=true'>>~/.termux/termux.properties</code><br>Reopen app");return};tx();n=0;w.loadUrl(U)}
private fun atlas(sz:Float):IntArray{val p=Paint().apply{textSize=sz;color=-1;isAntiAlias=true;typeface=Typeface.MONOSPACE};val cw=p.measureText("M").toInt()+1;val ch=(-p.ascent()+p.descent()).toInt()+1;val b=Bitmap.createBitmap(cw*95,ch,Bitmap.Config.ARGB_8888);Canvas(b).let{c->for(i in 0 until 95)c.drawText(((32+i).toChar()).toString(),(i*cw).toFloat(),-p.ascent(),p)};val r=IntArray(2+cw*95*ch);r[0]=cw;r[1]=ch;b.getPixels(r,2,cw*95,0,0,cw*95,ch);b.recycle();return r}
@android.annotation.SuppressLint("ClickableViewAccessibility")
override fun onCreate(b:Bundle?){super.onCreate(b)
w=WebView(this).apply{settings.javaScriptEnabled=true;addJavascriptInterface(this@M,"A")
webViewClient=object:WebViewClient(){override fun onReceivedError(v:WebView,r:WebResourceRequest,e:WebResourceError){if(r.isForMainFrame){if(n++<10){pg("<h2>Starting...</h2>attempt $n/10");if(n%3==0)tx();h.postDelayed({v.loadUrl(U)},2000)}else pg("<h2>Not responding</h2><button onclick='A.copy()'>Copy: a ui on</button> <button onclick='A.termux()'>Open Termux</button><br><br><button onclick='A.retry()'>Retry</button>")}}}}
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
