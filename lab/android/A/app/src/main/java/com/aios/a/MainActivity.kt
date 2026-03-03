package com.aios.a
import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.content.pm.PackageManager
import android.webkit.JavascriptInterface
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.webkit.WebView
import android.webkit.WebViewClient

private const val URL = "http://localhost:1111"
private fun pg(t: String, b: String) =
    "<body style='font:18px monospace;padding:40px;background:#1a1a1a;color:#0f0'><h2>$t</h2><p style='color:#ccc'>$b</p>"

class MainActivity : Activity() {
    private lateinit var wv: WebView
    private val h = Handler(Looper.getMainLooper())
    private var tries = 0
    private var booted = false

    private fun runCmd() {
        try { startForegroundService(Intent().apply {
            setClassName("com.termux", "com.termux.app.RunCommandService")
            action = "com.termux.RUN_COMMAND"
            putExtra("com.termux.RUN_COMMAND_PATH", "/data/data/com.termux/files/usr/bin/bash")
            putExtra("com.termux.RUN_COMMAND_ARGUMENTS", arrayOf("-l", "-c", "a ui on"))
            putExtra("com.termux.RUN_COMMAND_BACKGROUND", true)
        }) } catch (_: Exception) {}
    }

    private fun show(html: String) = wv.loadDataWithBaseURL(null, html, "text/html", "utf-8", null)

    inner class Bridge {
        @JavascriptInterface fun copy() {
            (getSystemService(CLIPBOARD_SERVICE) as ClipboardManager)
                .setPrimaryClip(ClipData.newPlainText("", "a ui on"))
        }
        @JavascriptInterface fun openTermux() {
            try { startActivity(Intent().apply {
                setClassName("com.termux", "com.termux.app.TermuxActivity")
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }) } catch (_: Exception) {}
        }
        @JavascriptInterface fun retry() { h.post { booted = false; boot() } }
    }

    private fun start(): String? {
        if (checkSelfPermission("com.termux.permission.RUN_COMMAND") != PackageManager.PERMISSION_GRANTED)
            return pg("Permission needed",
                "1. Settings → Apps → <b>a apk</b> → Permissions<br>→ enable <b>Run commands in Termux</b><br><br>" +
                "2. In Termux run:<br><code style='color:#ff0'>echo 'allow-external-apps=true' >> ~/.termux/termux.properties</code><br><br>Then reopen this app")
        runCmd()
        return null
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() { if (wv.canGoBack()) wv.goBack() else super.onBackPressed() }
    override fun onResume() { super.onResume(); if (!booted) boot() }

    private fun boot() {
        val e = start()
        if (e != null) { show(e); return }
        booted = true; tries = 0
        wv.loadUrl(URL)
    }

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        wv = WebView(this).apply {
            settings.javaScriptEnabled = true
            addJavascriptInterface(Bridge(), "A")
            webViewClient = object : WebViewClient() {
                @Suppress("DEPRECATION")
                override fun onReceivedError(v: WebView, c: Int, d: String?, u: String?) {
                    if (tries++ < 10) {
                        show(pg("Starting a ui...", "attempt $tries/10"))
                        if (tries % 3 == 0) runCmd()
                        h.postDelayed({ v.loadUrl(URL) }, 2000)
                    } else show(pg("Server not responding",
                        "<style>button{font:18px monospace;padding:12px 24px;margin:8px 8px 8px 0;border:2px solid #0f0;background:#222;color:#0f0;border-radius:8px}</style>" +
                        "<button onclick='A.copy()'>Copy: a ui on</button><button onclick='A.openTermux()'>Open Termux</button><br><br>" +
                        "<button onclick='A.retry()'>Retry</button>"))
                }
            }
        }
        setContentView(wv)
    }
}
