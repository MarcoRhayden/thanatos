param(
    [string]$Dir = "$PSScriptRoot\..\ .vcpkg"
)

$Dir = [System.IO.Path]::GetFullPath($Dir)

Write-Host "vcpkg target dir: $Dir"

$git = (Get-Command git -ErrorAction SilentlyContinue)
if (-not $git) {
    throw "Git não está instalado ou não está no PATH. Instale o Git e rode novamente."
}

if (-not (Test-Path -LiteralPath $Dir)) {
    Write-Host "Clonando vcpkg..."
    git clone --depth 1 https://github.com/microsoft/vcpkg.git "$Dir"
}
else {
    Write-Host "vcpkg já existe em $Dir"
}

# Bootstrap
$bootstrap = Join-Path $Dir "bootstrap-vcpkg.bat"
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "Arquivo de bootstrap não encontrado: $bootstrap"
}

Write-Host "Executando bootstrap: $bootstrap"
& "$bootstrap"

$resolved = (Resolve-Path -LiteralPath $Dir).Path
$env:VCPKG_ROOT = $resolved
Write-Host "VCPKG_ROOT set to $env:VCPKG_ROOT"
