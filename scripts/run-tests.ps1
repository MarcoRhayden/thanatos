param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x86",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot/.."
$BuildDir = Join-Path $Root "build-tests"
$Generator = "Visual Studio 17 2022"
$CMakeArch = if ($Arch -eq "x86") { "Win32" } else { "x64" }
$Triplet = if ($Arch -eq "x86") { "x86-windows" } else { "x64-windows" }

# Detect VCPKG_ROOT (env or .vcpkg local)
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot -and (Test-Path (Join-Path $Root ".vcpkg"))) {
    $VcpkgRoot = Resolve-Path (Join-Path $Root ".vcpkg")
}
if (-not $VcpkgRoot) {
    throw "VCPKG_ROOT not found. Set the VCPKG_ROOT env or clone vcpkg into '.vcpkg'."
}

$Toolchain = Join-Path $VcpkgRoot "scripts/buildsystems/vcpkg.cmake"

# Clean build-tests if it's the first time or if requested -Clean
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "==> Cleaning up build-tests..."
    Remove-Item -Recurse -Force $BuildDir
}
if (!(Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }

Write-Host "==> VCPKG_ROOT: $VcpkgRoot"
Write-Host "==> Configuring CMake ($Arch, $Config) ..."
cmake -S $Root -B $BuildDir -G "$Generator" -T host=x86 -A $CMakeArch `
    -DBUILD_TESTING=ON `
    -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
    -DVCPKG_TARGET_TRIPLET="$Triplet" `
    -DVCPKG_FEATURE_FLAGS="manifests"

Write-Host "==> Compiling ($Config) ..."
cmake --build $BuildDir --config $Config

Write-Host "==> Running tests..."
ctest --test-dir $BuildDir -C $Config --output-on-failure
