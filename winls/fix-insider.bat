@echo off
net session >nul 2>&1 || (powershell -Command "Start-Process '%~f0' -Verb RunAs" & exit)
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\Applicability" /v BranchName /t REG_SZ /d CanaryChannel /f
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\Applicability" /v Ring /t REG_SZ /d External /f
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\Applicability" /v ContentType /t REG_SZ /d Mainline /f
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\UI\Selection" /v UIBranch /t REG_SZ /d CanaryChannel /f
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\UI\Selection" /v UIRing /t REG_SZ /d External /f
reg add "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\UI\Selection" /v UIContentType /t REG_SZ /d Mainline /f
echo Done. Values set to CanaryChannel.
reg query "HKLM\SOFTWARE\Microsoft\WindowsSelfHost\Applicability" /v BranchName
pause
