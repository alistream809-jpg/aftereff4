# Collects AfterTrue.exe + Qt + FFmpeg + C++ runtime into dist/AfterTrue,
# makes a portable ZIP and (if Inno Setup is installed) the installer EXE.
param(
    [string]$BuildDir = "build",
    [string]$Version = ""
)
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $root

if (-not $Version) {
    if ($env:GITHUB_REF -like "refs/tags/v*") { $Version = $env:GITHUB_REF_NAME.TrimStart("v") }
    else {
        $m = Select-String -Path "CMakeLists.txt" -Pattern 'project\(AfterTrue VERSION (\d+\.\d+\.\d+)'
        $Version = $m.Matches[0].Groups[1].Value
    }
}
Write-Host "Packaging AFTER TRUE version $Version"

$exe = Join-Path $BuildDir "bin/Release/AfterTrue.exe"
if (-not (Test-Path $exe)) { throw "AfterTrue.exe not found at $exe - did the build succeed?" }

$dist = Join-Path $root "dist"
$app = Join-Path $dist "AfterTrue"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force $app | Out-Null
Copy-Item $exe $app

# --- FFmpeg DLLs (installed by vcpkg) ---
$av = Get-ChildItem -Path $BuildDir -Recurse -Filter "avcodec*.dll" |
      Where-Object { $_.FullName -notmatch '[\\/]debug[\\/]' } | Select-Object -First 1
if (-not $av) { throw "FFmpeg DLLs (avcodec*.dll) not found under $BuildDir" }
Copy-Item (Join-Path $av.DirectoryName "*.dll") $app
Write-Host "Copied FFmpeg DLLs from $($av.DirectoryName)"

# --- Qt DLLs and plugins ---
$deploy = $null
if ($env:QT_ROOT_DIR) { $c = Join-Path $env:QT_ROOT_DIR "bin/windeployqt.exe"; if (Test-Path $c) { $deploy = $c } }
if (-not $deploy) { $cmd = Get-Command windeployqt -ErrorAction SilentlyContinue; if ($cmd) { $deploy = $cmd.Source } }
if (-not $deploy) { throw "windeployqt.exe not found (is Qt installed?)" }
& $deploy --release --no-translations (Join-Path $app "AfterTrue.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# --- Microsoft C++ runtime (so the app also starts on clean PCs) ---
$crt = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT" -Directory -ErrorAction SilentlyContinue |
       Sort-Object FullName -Descending | Select-Object -First 1
if ($crt) { Copy-Item (Join-Path $crt.FullName "*.dll") $app; Write-Host "Copied C++ runtime from $($crt.FullName)" }
else { Write-Warning "Visual C++ runtime DLLs not found; users may need the VC++ Redistributable." }

Copy-Item "README.md" $app -ErrorAction SilentlyContinue
Copy-Item "docs/FEATURES.md" $app -ErrorAction SilentlyContinue
Copy-Item "docs/THIRD_PARTY.md" $app -ErrorAction SilentlyContinue

# --- Portable ZIP ---
$zip = Join-Path $dist "AFTER-TRUE-Portable-$Version.zip"
Compress-Archive -Path $app -DestinationPath $zip
Write-Host "Created $zip"

# --- Installer ---
$iscc = @("C:\Program Files (x86)\Inno Setup 6\ISCC.exe", "C:\Program Files\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { $cmd = Get-Command iscc -ErrorAction SilentlyContinue; if ($cmd) { $iscc = $cmd.Source } }
if ($iscc) {
    & $iscc "/DAppVersion=$Version" "/DSourceDir=$app" "/DOutDir=$dist" "installer/AfterTrue.iss"
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }
    Write-Host "Installer created in $dist"
} else {
    Write-Warning "Inno Setup not found - skipped installer (portable ZIP is still available)."
}
Get-ChildItem $dist | Format-Table Name, Length
