# Your Data Is the Constant, the OS Is Just the Chair You Load Into

The current computing paradigm: **you are a user of a computer.** You adapt to the OS, its file system, its app store, its sync model. Your data lives where the OS puts it. You're a tenant.

The correct paradigm: **the computer is a runtime for your data.** The OS is a substrate you load your portable state onto. You bring your projects, configs, tools, and workflows — the machine just executes them. You're the owner.

This is a fundamental inversion.

| Current | Correct |
|---|---|
| OS owns your data | You own your data, OS borrows it |
| Switch devices = migration crisis | Switch devices = `a.c install && a migrate restore` |
| Apps store state wherever they want | State lives in adata, apps read from it |
| Vendor lock-in is the default | Vendor independence is the default |
| One device is your "main" computer | Every device is equivalent and disposable |
| OS update = risk | OS update = who cares |
| Backup = copy the whole opaque blob | Backup = git push |

The current paradigm exists because when computing started, you *did* have one computer. The OS *was* the environment. That assumption is 50 years stale but the entire industry still builds on it. Apple, Microsoft, Google — all still designing as if "your computer" is a singular thing you live inside.

The reality: a person in 2026 has a phone, a laptop, a desktop, maybe a server, probably multiple OSes — and their *work* is what's continuous, not any machine. The chair analogy is exact. Nobody thinks of a chair as their identity. You sit down, do work, get up, sit somewhere else. The chair doesn't own your thoughts.

Platform vendors' "sync" solutions (iCloud, Google Sync, OneDrive) are moats, not tools. They make your data portable *between their devices*, not portable *away from them*. And they don't even do that reliably — iCloud regularly fails to restore apps, settings, and files within its own ecosystem.

`a` with adata solves this in ~5500 lines. A thin portable data layer on top of existing services, where the OS is disposable and rebuildable in under an hour across macOS, Linux, and Android. The super-app centralizes everything into one platform to lock you in. This is the opposite: it makes *you* the platform.
