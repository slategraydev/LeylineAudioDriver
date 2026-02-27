# Leyline Audio C++: Simplified & Robust Installer
# Logic: Build (Host) -> Deploy & Verify (VM)
# Mirrors Install.ps1 from the Rust project, adapted for C++ / NMAKE builds.

param (
    [switch]$clean,        # Full clean build on HOST
    [switch]$fast,         # Skip reverting VM
    [switch]$Uninstall,    # Only perform uninstallation on VM
    [string]$VMName = ($env:LEYLINE_VM_NAME -or "LeylineTestVM"),
    [string]$SnapshotName = ($env:LEYLINE_VM_SNAPSHOT -or "LeylineSnapshot"),
    [PSCredential]$Credential
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
$remotePath = "C:\LeylineInstall"
$BuildVersion = "0.2.0"

# --- Credentials ---
if (-not $PSBoundParameters.ContainsKey('Credential')) {
    $VMUser = ($env:LEYLINE_VM_USER -or "USER")
    $VMPassword = ($env:LEYLINE_VM_PASS -or "REDACTED_VM_PASS")
    $secPassword = ConvertTo-SecureString $VMPassword -AsPlainText -Force
    $Credential = New-Object System.Management.Automation.PSCredential ($VMUser, $secPassword)
}

# --- 0. CERTIFICATE GENERATION ---
$pfxPath = "$ProjectRoot\leyline.pfx"
$cerPath = "$ProjectRoot\leyline.cer"
$CertPassword = ($env:LEYLINE_CERT_PASS -or "REDACTED_CERT_PASS")

if (-not (Test-Path $pfxPath)) {
    Write-Host "[*] Generating Self-Signed Certificate..."
    $cert = New-SelfSignedCertificate -Subject "Leyline Audio" -Type CodeSigningCert -CertStoreLocation "Cert:\CurrentUser\My"
    $cert | Export-PfxCertificate -FilePath $pfxPath -Password (ConvertTo-SecureString -String $CertPassword -Force -AsPlainText)
    $cert | Export-Certificate    -FilePath $cerPath
}

# --- 1. VM SNAPSHOT HANDLING ---
if (-not $fast -and -not $Uninstall) {
    Write-Host "[*] Reverting VM '$VMName' to snapshot '$SnapshotName'..." -ForegroundColor Cyan
    Try {
        Restore-VMSnapshot -VMName $VMName -Name $SnapshotName -Confirm:$false
        Start-VM -Name $VMName -ErrorAction SilentlyContinue
    }
    Catch {
        Write-Warning "Snapshot revert failed. Ensure the VM and snapshot exist."
        Return
    }

    Write-Host "    Waiting for VM network..." -NoNewline
    $timeout = 60
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    while ($timer.Elapsed.TotalSeconds -lt $timeout) {
        $vm = Get-VM -Name $VMName
        if ($vm.State -eq 'Running' -and $vm.NetworkAdapters[0].IpAddresses.Count -gt 0) {
            Write-Host " Ready." -ForegroundColor Green; break
        }
        Start-Sleep -Seconds 1; Write-Host "." -NoNewline
    }
    Start-Sleep -Seconds 5
}

# --- 2. HOST BUILD ---
if (-not $Uninstall) {
    Write-Host "[*] [HOST] Building Leyline C++ $BuildVersion..." -ForegroundColor Cyan
    . "$PSScriptRoot\LaunchBuildEnv.ps1"

    if ($clean) {
        Write-Host "    -> Cleaning build artifacts..."
        $targets = @("$ProjectRoot\driver\objfre_win10_amd64", "$ProjectRoot\package")
        foreach ($t in $targets) { if (Test-Path $t) { Remove-Item $t -Recurse -Force } }
    }

    # Build with NMAKE inside the eWDK environment.
    Push-Location "$ProjectRoot\driver"
    if ($env:NMAKE_EXE -and (Test-Path $env:NMAKE_EXE)) {
        & $env:NMAKE_EXE /f makefile.def BUILD_ALT_DIR=fre BUILD_DEFAULT_TARGETS=1
    }
    else {
        # Fallback: invoke build.exe (legacy eWDK).
        build -cz
    }
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw "Kernel build failed." }
    Pop-Location

    # Stage package artifacts.
    if (-not (Test-Path "$ProjectRoot\package")) { New-Item -ItemType Directory -Path "$ProjectRoot\package" | Out-Null }

    # Copy built SYS and generated INF (Inf2Cat will produce the CAT).
    $buildOutput = "$ProjectRoot\driver\objfre_win10_amd64\amd64"
    if (Test-Path "$buildOutput\leyline.sys") {
        Copy-Item "$buildOutput\leyline.sys" "$ProjectRoot\package\leyline.sys" -Force
    }
    Copy-Item "$ProjectRoot\driver\leyline.inx" "$ProjectRoot\package\leyline.inf" -Force

    # Run Inf2Cat to produce the catalog.
    if ($env:INF2CAT_EXE -and (Test-Path $env:INF2CAT_EXE)) {
        & $env:INF2CAT_EXE /driver:"$ProjectRoot\package" /os:10_X64 | Out-Null
    }

    # Sign.
    $signArgs = @("sign", "/f", $pfxPath, "/p", $CertPassword, "/fd", "SHA256")
    if (Test-Path "$ProjectRoot\package\leyline.sys") {
        & $env:SIGNTOOL_EXE $signArgs "$ProjectRoot\package\leyline.sys" | Out-Null
    }
    if (Test-Path "$ProjectRoot\package\leyline.cat") {
        & $env:SIGNTOOL_EXE $signArgs "$ProjectRoot\package\leyline.cat" | Out-Null
    }

    # Ensure cert + devcon land in the package.
    Copy-Item $cerPath "$ProjectRoot\package\leyline.cer" -Force
    $devconHostPath = "D:\eWDK_28000\Program Files\Windows Kits\10\Tools\10.0.28000.0\x64\devcon.exe"
    if (Test-Path $devconHostPath) {
        Copy-Item $devconHostPath "$ProjectRoot\package\devcon.exe" -Force
    }
}

# --- 3. VM DEPLOY & VERIFY ---
try {
    Write-Host "[*] [VM] Connecting and Installing..." -ForegroundColor Cyan
    $vmsess = New-PSSession -VMName $VMName -Credential $Credential
    $sdkVersion = if ($env:LEYLINE_SDK_VERSION) { $env:LEYLINE_SDK_VERSION } else { "10.0.28000.0" }

    Invoke-Command -Session $vmsess -ScriptBlock {
        param($path, $isUninstall)
        $ErrorActionPreference = "Continue"
        Write-Host "    (VM) Preparing environment..."
        if ($isUninstall) {
            pnputil /remove-device "ROOT\MEDIA\LeylineAudio" /force | Out-Null
            $drivers = pnputil /enum-drivers | Select-String "Original Name:\s+leyline.inf" -Context 3, 0
            foreach ($d in $drivers) {
                if ($d.Context.PreContext[0] -match "Published Name:\s+(oem\d+\.inf)") {
                    pnputil /delete-driver $matches[1] /uninstall /force
                }
            }
            return
        }
        if (Test-Path $path) { Remove-Item $path -Recurse -Force }
        New-Item -ItemType Directory -Path $path -Force | Out-Null
    } -ArgumentList $remotePath, $Uninstall

    if ($Uninstall) { Write-Host "[SUCCESS] Uninstalled."; return }

    Copy-Item -Path "$ProjectRoot\package\*" -Destination $remotePath -ToSession $vmsess -Recurse -Force

    Invoke-Command -Session $vmsess -ScriptBlock {
        param($path, $sdkVersion)
        Set-Location $path
        certutil -addstore -f root             leyline.cer | Out-Null
        certutil -addstore -f TrustedPublisher leyline.cer | Out-Null

        Write-Host "    (VM) Upgrading Driver Stack..."
        pnputil /add-driver "leyline.inf" /install | Out-Null

        $devcon = "C:\eWDK_28000\Program Files\Windows Kits\10\Tools\$sdkVersion\x64\devcon.exe"
        if (Test-Path $devcon) {
            & $devcon update "leyline.inf" "ROOT\MEDIA\LeylineAudio" | Out-Null
        }
        else {
            Write-Host "    (VM) [WARNING] devcon.exe not found. Falling back to devgen..." -ForegroundColor Yellow
            devgen /add /bus ROOT /hardwareid "ROOT\MEDIA\LeylineAudio" | Out-Null
        }

        Start-Sleep -Seconds 3
        $device = Get-PnpDevice | Where-Object { $_.HardwareId -match "ROOT\\MEDIA\\LeylineAudio" } | Select-Object -First 1
        if ($device) {
            Write-Host "    (VM) UPGRADE SUCCESSFUL: $($device.InstanceId)" -ForegroundColor Green
            Write-Host "    (VM) Status: $($device.Status)"
            Restart-Service "AudioEndpointBuilder" -Force -ErrorAction SilentlyContinue
            Restart-Service "Audiosrv"             -Force -ErrorAction SilentlyContinue
        }
        else {
            Write-Host "    (VM) No existing device found. Performing fresh install..."
            if (Test-Path $devcon) {
                & $devcon install "leyline.inf" "ROOT\MEDIA\LeylineAudio" | Out-Null
            }
            else {
                devgen /add /bus ROOT /hardwareid "ROOT\MEDIA\LeylineAudio" | Out-Null
            }
        }

        Write-Host "    (VM) Searching for Endpoints..."
        $foundCount = 0
        $endpoints = Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render" -ErrorAction SilentlyContinue
        foreach ($e in $endpoints) {
            $prop = Join-Path $e.Name "Properties"
            if (Test-Path "Registry::$prop") {
                $val = (Get-ItemProperty "Registry::$prop")."{a45c254e-df1c-4efd-8020-67d146a850e0},2"
                if ($val -like "*Leyline*") {
                    Write-Host "    (VM) Found Endpoint: $val" -ForegroundColor Green
                    $foundCount++
                }
            }
        }
        Write-Host "    (VM) Total Leyline Audio Endpoints: $foundCount"
    } -ArgumentList $remotePath, $sdkVersion
}
catch {
    Write-Error "VM Operation failed: $_"
}
finally {
    if ($vmsess) { Remove-PSSession $vmsess }
    Write-Host "[*] Done."
}
