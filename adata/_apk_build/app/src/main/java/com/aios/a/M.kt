@file:Suppress("DEPRECATION","OVERRIDE_DEPRECATION")
package com.aios.a
import android.app.Activity;import android.content.*;import android.os.*;import android.webkit.*
private const val U="http://localhost:1111";private const val T="com.termux"
class M:Activity(){
private lateinit var w:WebView;private val h=Handler(Looper.getMainLooper());private var n=0
private fun tx(){try{startForegroundService(Intent().apply{setClassName(T,"$T.app.RunCommandService");action="$T.RUN_COMMAND";putExtra("$T.RUN_COMMAND_PATH","/data/data/$T/files/usr/bin/bash");putExtra("$T.RUN_COMMAND_ARGUMENTS",arrayOf("-l","-c","a ui on"));putExtra("$T.RUN_COMMAND_BACKGROUND",true)})}catch(_:Exception){}}
private fun pg(s:String)=w.loadDataWithBaseURL(null,"<body style='font:18px monospace;padding:20px;background:#000;color:#0f0'>$s","text/html","utf-8",null)
@JavascriptInterface fun copy(){(getSystemService(CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("","a ui on"))}
@JavascriptInterface fun termux(){try{startActivity(Intent().apply{setClassName(T,"$T.app.TermuxActivity");addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)})}catch(_:Exception){}}
@JavascriptInterface fun retry(){h.post{boot()}}
override fun onBackPressed(){if(w.canGoBack())w.goBack() else super.onBackPressed()}
override fun onResume(){super.onResume();boot()}
private fun boot(){if(checkSelfPermission("$T.permission.RUN_COMMAND")!=0){pg("<h2>Permission needed</h2>Settings→Apps→a→enable Run commands in Termux<br><br>In Termux:<br><code style='color:#ff0'>echo 'allow-external-apps=true'>>~/.termux/termux.properties</code><br>Reopen app");return};tx();n=0;w.loadUrl(U)}
override fun onCreate(b:Bundle?){super.onCreate(b)
w=WebView(this).apply{settings.javaScriptEnabled=true;addJavascriptInterface(this@M,"A")
webViewClient=object:WebViewClient(){override fun onReceivedError(v:WebView,r:WebResourceRequest,e:WebResourceError){if(r.isForMainFrame){if(n++<10){pg("<h2>Starting...</h2>attempt $n/10");if(n%3==0)tx();h.postDelayed({v.loadUrl(U)},2000)}else pg("<h2>Not responding</h2><button onclick='A.copy()'>Copy: a ui on</button> <button onclick='A.termux()'>Open Termux</button><br><br><button onclick='A.retry()'>Retry</button>")}}
override fun onRenderProcessGone(v:WebView,d:RenderProcessGoneDetail):Boolean{w=WebView(this@M).apply{settings.javaScriptEnabled=true;addJavascriptInterface(this@M,"A");webViewClient=this@M.w.webViewClient};setContentView(w);boot();return true}}};setContentView(w)}}
