param(
    [string]$Dir
)

if (-not $PSBoundParameters.ContainsKey('Dir') -or [string]::IsNullOrWhiteSpace($Dir)) {
    $repoRoot = Resolve-Path "$PSScriptRoot\.."
    $Dir = Join-Path $repoRoot ".vcpkg"
}

$Dir = [System.IO.Path]::GetFullPath($Dir)
Write-Host "vcpkg target dir: $Dir"

$git = (Get-Command git -ErrorAction SilentlyContinue)
if (-not $git) {
    throw "Git is not installed or is not in the PATH. Install Git and run again."
}

if (-not (Test-Path -LiteralPath $Dir)) {
    Write-Host "Cloning vcpkg into $Dir ..."
    git clone --depth 1 https://github.com/microsoft/vcpkg.git "$Dir"
}
else {
    Write-Host "vcpkg already exists in $Dir"
    if (Test-Path -LiteralPath (Join-Path $Dir ".git")) {
        Write-Host "Updating vcpkg (git pull)..."
        git -C "$Dir" pull --ff-only | Out-Host
    }
}

$bootstrap = Join-Path $Dir "bootstrap-vcpkg.bat"
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "Bootstrap file not found: $bootstrap"
}

Write-Host "Running bootstrap: $bootstrap"
& "$bootstrap" -disableMetrics

$resolved = (Resolve-Path -LiteralPath $Dir).Path
$env:VCPKG_ROOT = $resolved
Write-Host "VCPKG_ROOT set to $env:VCPKG_ROOT"

Write-Host ""
Write-Host "Use com CMake:"
Write-Host "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=""$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"""
Write-Host "  cmake --build build --config Release"
