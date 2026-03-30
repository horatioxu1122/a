@echo off
net session >nul 2>&1 || (powershell -Command "Start-Process '%~f0' -Verb RunAs" & exit)
echo === Windows Update ===
usoclient StartInstallWait
echo === Winget Updates ===
winget upgrade --all --accept-source-agreements --accept-package-agreements
echo Done.
pause
