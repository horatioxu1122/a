"""a vm [run|ssh|kill] — disposable QEMU VM for safe testing"""
import sys,os,subprocess as S,time
D=os.path.expanduser("~/a/adata/vm");IMG=D+"/test.qcow2";PORT="2222";P="testvm1"
SO=["sshpass","-p",P,"ssh","-o","StrictHostKeyChecking=no","-o","UserKnownHostsFile=/dev/null","-o","ConnectTimeout=10","-o","ServerAliveInterval=10","-p",PORT,"debian@localhost"]
def setup():
    os.makedirs(D,exist_ok=True)
    if not os.path.exists(IMG):
        print("> downloading debian cloud image...")
        S.run(["curl","-fsSLo",IMG,"https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-genericcloud-amd64.qcow2"],check=True)
        S.run(["qemu-img","resize",IMG,"10G"],check=True)
        open(D+"/user-data","w").write(f"#cloud-config\npassword: {P}\nchpasswd: {{expire: false}}\nssh_pwauth: true\npackages: [git,curl,tmux,openssh-server]\nruncmd: [systemctl enable --now ssh]\n")
        os.makedirs(D+"/seed",exist_ok=True);open(D+"/seed/user-data","w").write(open(D+"/user-data").read());open(D+"/seed/meta-data","w").write('{"instance-id":"vm0"}')
        S.run(["mkisofs","-o",D+"/seed.iso","-V","cidata","-J","-r",D+"/seed/"],check=True)
cmd=sys.argv[2] if len(sys.argv)>2 else "run"
if cmd=="run":
    setup();S.Popen(["qemu-system-x86_64","-m","2G","-smp","2","-drive",f"file={IMG},format=qcow2","-drive",f"file={D}/seed.iso,format=raw","-device","virtio-net-pci,netdev=n0","-netdev",f"user,id=n0,hostfwd=tcp::{PORT}-:22","-nographic","-enable-kvm"],stdout=open(D+"/console.log","w"),stderr=S.STDOUT)
    print(f"booting...")
    for i in range(12):
        time.sleep(10)
        if S.run(SO+["echo ready"],capture_output=True).returncode==0:print("ready");sys.exit(0)
        print(f"  waiting ({(i+1)*10}s)...")
    print(f"x timeout — check {D}/console.log")
elif cmd=="ssh":os.execvp(SO[0],SO+sys.argv[3:])
elif cmd=="kill":S.run(["pkill","-f",f"hostfwd=tcp::{PORT}"]) and None;print("✓")
