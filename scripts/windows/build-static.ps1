param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",

    [ValidateSet("x86", "x64")]
    [string]$Arch = "x86",

    # Limpa a pasta de build antes
    [switch]$Clean,

    # Forçar usar o vcpkg local (.vcpkg) em vez do global (VCPKG_ROOT)
    [switch]$UseLocalVcpkg,

    # Faz fetch/checkout+bootstrap do vcpkg antes de configurar
    [switch]$BootstrapVcpkg = $true,

    # >>> Nome do target/EXE (sem .exe). Default: Thanatos
    [string]$TargetName = "Thanatos"
)

$ErrorActionPreference = "Stop"
function Fail($msg) { Write-Error $msg; exit 1 }

# --- Paths / generator
$Root = Resolve-Path "$PSScriptRoot/.."
$Gen = "Visual Studio 17 2022"
$ArchMap = @{ x86 = "Win32"; x64 = "x64" }
$VSArch = $ArchMap[$Arch]
$Triplet = if ($Arch -eq "x86") { "x86-windows-static" } else { "x64-windows-static" }
$BuildDir = Join-Path $Root "build/$VSArch/$Config-static"

# --- Enter VS dev environment (MSBuild toolset)
function Enter-VSDevShell {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path $vswhere)) { return }
    $vs = & $vswhere -latest -requires Microsoft.Component.MSBuild -property installationPath 2>$null
    if (-not $vs) { return }
    $vsDevCmd = Join-Path $vs "Common7\Tools\VsDevCmd.bat"
    if (Test-Path $vsDevCmd) {
        Write-Host "==> Loading VS environment ($VSArch)..."
        & cmd /c "`"$vsDevCmd`" -arch=$Arch" | Out-Null
    }
}
Enter-VSDevShell

# --- Resolve vcpkg root
$VcpkgRoot = $null
if ($UseLocalVcpkg) {
    if (Test-Path (Join-Path $Root ".vcpkg")) { $VcpkgRoot = Resolve-Path (Join-Path $Root ".vcpkg") }
    else { Fail "'.vcpkg' not found. Remove -UseLocalVcpkg or clone vcpkg into '.vcpkg'." }
}
else {
    if ($env:VCPKG_ROOT) { $VcpkgRoot = $env:VCPKG_ROOT }
    elseif (Test-Path (Join-Path $Root ".vcpkg")) { $VcpkgRoot = Resolve-Path (Join-Path $Root ".vcpkg") }
}
if (-not $VcpkgRoot) { Fail "VCPKG_ROOT not found. Set VCPKG_ROOT or clone vcpkg into '.vcpkg'." }
$Toolchain = Join-Path $VcpkgRoot "scripts/buildsystems/vcpkg.cmake"

# --- (opt) bootstrap/update vcpkg
if ($BootstrapVcpkg) {
    Write-Host "==> Preparing vcpkg at $VcpkgRoot ..."
    $gitExe = (Get-Command git -ErrorAction SilentlyContinue)
    if ($gitExe) {
        try {
            git -C $VcpkgRoot fetch --prune --tags --force --depth=0 | Out-Null
            git -C $VcpkgRoot checkout master 2>$null | Out-Null
            git -C $VcpkgRoot pull | Out-Null
        }
        catch { Write-Host "   (warn) git update failed, continuing..." }
    }
    $bootstrap = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
    if (Test-Path $bootstrap) { & $bootstrap | Out-Null } else { Write-Host "   (warn) no bootstrap-vcpkg.bat" }
}

# --- Clean
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "==> Cleaning $BuildDir ..."
    Remove-Item -Recurse -Force $BuildDir
}

# --- Configure
Write-Host "==> Configuring CMake ($Arch $Config, triplet=$Triplet) ..."
$cmakeArgs = @(
    "-S", $Root,
    "-B", $BuildDir,
    "-G", $Gen, "-T", "host=x86", "-A", $VSArch,
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
    "-DVCPKG_TARGET_TRIPLET=$Triplet",
    "-DVCPKG_FEATURE_FLAGS=manifests",
    "-DVCPKG_APPLOCAL_DEPS=OFF",
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`$<`$<CONFIG:Debug`>:Debug`>",
    "-DBUILD_TESTING=OFF"
)
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Fail 'CMake configure failed.' }

# --- Build
Write-Host "==> Building ..."
cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { Fail 'Build failed.' }

# --- Result (try specific target name first, fallback to any .exe)
$exe = Join-Path $BuildDir "$Config\$TargetName.exe"
if (-not (Test-Path $exe)) {
    $cand = Get-ChildItem -Path (Join-Path $BuildDir $Config) -Filter *.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cand) { $exe = $cand.FullName }
}

if (Test-Path $exe) {
    $sizeMB = [Math]::Round((Get-Item $exe).Length / 1MB, 2)
    Write-Host "==> OK: $exe ($sizeMB MB)"
    Write-Host "Run it like this (with an absolute TOML path):"
    Write-Host "    `"$exe`" --config `"$((Resolve-Path "$Root/config/thanatos.toml").Path)`""
    if (-not $UseLocalVcpkg -and $env:VCPKG_ROOT) {
        Write-Host "Using VCPKG_ROOT = $($env:VCPKG_ROOT)"
    }
    else {
        Write-Host "Using vcpkg at    = $VcpkgRoot"
    }
}
else {
    Fail "Executable not found in $(Join-Path $BuildDir $Config)"
}
