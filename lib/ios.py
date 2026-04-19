"""a ios — iPhone access without Developer Mode (Dev Mode recommended: Settings→Privacy→Developer Mode enables `xcrun devicectl` for full capability). a ios [id|info|log|apps|backup DIR|mount DIR]"""
import sys,subprocess as S,os,shutil
shutil.which("ideviceinfo") or [S.call(["brew","install",p]) for p in ("libimobiledevice","ideviceinstaller","ifuse")]
a=sys.argv[2:] or ["id"]
d=a[1] if len(a)>1 else {"backup":"./iphone-backup","mount":os.path.expanduser("~/iphone")}.get(a[0],"")
a[0]=="mount" and os.makedirs(d,exist_ok=True)
S.call({"id":["idevice_id","-l"],"info":["ideviceinfo"],"log":["idevicesyslog"],"apps":["ideviceinstaller","list"],"backup":["idevicebackup2","backup",d],"mount":["ifuse",d]}.get(a[0],["idevice_"+a[0]]+a[1:]))
