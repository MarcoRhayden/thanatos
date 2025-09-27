param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x86",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
function Fail($msg) { Write-Error $msg; exit 1 }

# --- Resolve paths
$Root = Resolve-Path "$PSScriptRoot/.."
$Gen = "Visual Studio 17 2022"
$ArchMap = @{ x86 = "Win32"; x64 = "x64" }
$Triplet = if ($Arch -eq "x86") { "x86-windows" } else { "x64-windows" }
$BuildDir = Join-Path $Root "build/$($ArchMap[$Arch])/$Config"

# --- Ensure VS dev environment
function Enter-VSDevShell {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path $vswhere)) { return }
    $vs = & $vswhere -latest -requires Microsoft.Component.MSBuild -property installationPath 2>$null
    if (-not $vs) { return }
    $vsDevCmd = Join-Path $vs "Common7\Tools\VsDevCmd.bat"
    if (Test-Path $vsDevCmd) {
        Write-Host "==> Carregando ambiente do VS..."
        & cmd /c "`"$vsDevCmd`" -arch=$Arch" | Out-Null
    }
}
Enter-VSDevShell

# --- VCPKG toolchain
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot -and (Test-Path (Join-Path $Root ".vcpkg"))) {
    $VcpkgRoot = Resolve-Path (Join-Path $Root ".vcpkg")
}
if (-not $VcpkgRoot) { Fail "VCPKG_ROOT não encontrado. Defina a env VCPKG_ROOT ou clone o vcpkg em '.vcpkg'." }
$Toolchain = Join-Path $VcpkgRoot "scripts/buildsystems/vcpkg.cmake"

# --- Clean
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "==> Limpando $BuildDir ..."
    Remove-Item -Recurse -Force $BuildDir
}

# --- Configure
Write-Host "==> Configurando CMake ($Arch $Config) ..."
cmake -S $Root -B $BuildDir `
    -G "$Gen" -T host=x86 -A $($ArchMap[$Arch]) `
    -DCMAKE_TOOLCHAIN_FILE="$Toolchain" -DVCPKG_TARGET_TRIPLET="$Triplet" -DVCPKG_FEATURE_FLAGS=manifests `
    -DBUILD_TESTING=OFF

if ($LASTEXITCODE -ne 0) { Fail 'CMake configure failed.' }

# --- Build
Write-Host "==> Compilando ..."
cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { Fail 'Build failed.' }