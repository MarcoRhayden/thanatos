param(
    [string]$Dir = "$PSScriptRoot\..\ .vcpkg"
)

$Dir = [System.IO.Path]::GetFullPath($Dir)

Write-Host "vcpkg target dir: $Dir"

$git = (Get-Command git -ErrorAction SilentlyContinue)
if (-not $git) {
    throw "Git is not installed or is not in the PATH. Install Git and run again."
}

if (-not (Test-Path -LiteralPath $Dir)) {
    Write-Host "Cloning vcpkg..."
    git clone --depth 1 https://github.com/microsoft/vcpkg.git "$Dir"
}
else {
    Write-Host "vcpkg already exists in $Dir"
}

# Bootstrap
$bootstrap = Join-Path $Dir "bootstrap-vcpkg.bat"
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "Bootstrap file not found: $bootstrap"
}

Write-Host "Running bootstrap: $bootstrap"
& "$bootstrap"

$resolved = (Resolve-Path -LiteralPath $Dir).Path
$env:VCPKG_ROOT = $resolved
Write-Host "VCPKG_ROOT set to $env:VCPKG_ROOT"
