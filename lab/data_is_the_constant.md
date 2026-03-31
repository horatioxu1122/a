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

## Why Dropbox Almost Got Here and Didn't

Dropbox had the distribution, the trust, and the cross-platform presence. Users literally told them what to build — "I want to back up my desktop, my apps." But Dropbox synced *content* without *context*. Your files are useless without the environment that makes them work — the tools, the settings, the configuration that makes a generic machine *yours*. When users said "sync my stuff" they meant everything. Dropbox heard "files." The gap between those two meanings is the entire product they never built.

The personalization layer is actually smaller and more tractable than the file layer. Migration data is a few hundred KB of JSON. Files are gigabytes. The hard valuable thing was the small cheap thing to store. They were already paying for infrastructure to sync the expensive part and never built the cheap part that would have made it all matter.

Then they went enterprise (~2015-2017), pivoting to selling to IT departments who want managed locked-down devices, not portable personal computing. And they flinched when Apple, Google, and Microsoft attacked them with built-in alternatives — retreating to commodity file sync instead of going deeper into system integration.

## Why Nobody Could Build This Until Now

Three blockers that all lifted at once:

1. **Storage/bandwidth.** In 2015 syncing configs was cheap but syncing everything around them wasn't. Now it's essentially free. But the configs were always cheap — they just couldn't see them separately from the file problem.

2. **Cross-platform knowledge.** This is the big one. To build cross-platform migration you need to know: brew on mac, apt/snap/flatpak on debian, pacman on arch, defaults/plist on mac, dconf/gsettings on gnome, where every app stores its config on each OS, how keychain encryption works, how each package manager handles repos and keys. No single person knows all of this. None of it is technically hard — it's that the knowledge is scattered across a thousand Stack Overflow posts, man pages, and tribal knowledge that no one person has ever unified. You can't Google your way to a cross-platform migration system because Google gives you one answer for one subsystem on one OS at a time.

3. **LLMs collapse the knowledge problem.** An LLM can hold apt repo keyring handling, brew cask artifact detection, and macOS defaults export all in context simultaneously and wire them together in one file. A human team would need 3-4 platform specialists who'd never agree on an architecture. An LLM treats it as one problem because it doesn't have the human specialization boundary. The cross-platform complexity that was organizationally impossible for a company to staff is trivial when knowledge isn't siloed in different people's heads.

Dropbox in 2015 would have needed a team of 10 platform specialists arguing for a year. One person and an LLM did it in an evening. The problem didn't get easier — the tool for solving it finally exists.
