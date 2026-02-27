# Leyline Audio C++: VM UNINSTALLER
# Thin wrapper that delegates to Install.ps1 -Uninstall.

param (
    [string]$VMName = ($env:LEYLINE_VM_NAME -or "LeylineTestVM")
)

Write-Host "[*] Triggering Consolidated VM Uninstall..." -ForegroundColor Cyan
& "$PSScriptRoot\Install.ps1" -Uninstall -VMName $VMName
