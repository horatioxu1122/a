# Fix Windows Insider enrollment - switch to Canary channel
# Must be run as Administrator

$app = "HKLM:\SOFTWARE\Microsoft\WindowsSelfHost\Applicability"
$ui = "HKLM:\SOFTWARE\Microsoft\WindowsSelfHost\UI\Selection"

Set-ItemProperty $app -Name BranchName -Value "CanaryChannel"
Set-ItemProperty $app -Name Ring -Value "External"
Set-ItemProperty $app -Name ContentType -Value "Mainline"
Set-ItemProperty $ui -Name UIBranch -Value "CanaryChannel"
Set-ItemProperty $ui -Name UIRing -Value "External"
Set-ItemProperty $ui -Name UIContentType -Value "Mainline"

Write-Host "Registry updated. Verifying..."
Get-ItemProperty $app | Select BranchName,Ring,ContentType | Format-List
Get-ItemProperty $ui | Select UIBranch,UIRing,UIContentType | Format-List
Write-Host "Reboot then: Settings > Windows Update > Check for updates"
Read-Host "Press Enter to reboot (or close window to skip)"
Restart-Computer
