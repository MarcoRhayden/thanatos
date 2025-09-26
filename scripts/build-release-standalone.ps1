param(
    [string]$Generator = 'Visual Studio 17 2022',
    [string]$Config = 'Release'
)

function Fail([string]$msg) {
    Write-Error $msg
    exit 1
}

# Check VCPKG_ROOT
if (-not $env:VCPKG_ROOT) {
    Fail 'VCPKG_ROOT not set. Run .\scripts\setup-vcpkg.ps1 first.'
}

$toolchain = Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake'
$triplet = 'x86-windows-static'

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildDir = Join-Path $root 'build\Win32\Release-static'
$outDir = Join-Path $root 'dist\standalone'

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $outDir  | Out-Null

Write-Host '>>> Build standalone (Win32/x86, static)'
Write-Host ('Build Dir : ' + $buildDir)
Write-Host ('Out Dir   : ' + $outDir)

# Configure (no backticks, ascii only)
$configureArgs = @(
    '-S', $root,
    '-B', $buildDir,
    '-G', $Generator,
    '-A', 'Win32',
    '-T', 'host=x86',
    ('-DCMAKE_BUILD_TYPE=' + $Config),
    ('-DCMAKE_TOOLCHAIN_FILE=' + $toolchain),
    ('-DVCPKG_TARGET_TRIPLET=' + $triplet),
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
)
cmake @configureArgs
if ($LASTEXITCODE -ne 0) { Fail 'CMake configure failed.' }

# Build
$buildArgs = @('--build', $buildDir, '--config', $Config)
cmake @buildArgs
if ($LASTEXITCODE -ne 0) { Fail 'CMake build failed.' }

# Executable: <buildDir>\Release\arkan_poseidon.exe
$exe = Join-Path $buildDir (Join-Path $Config 'arkan_poseidon.exe')
if (-not (Test-Path -LiteralPath $exe)) {
    Fail ('Executable not found at: ' + $exe)
}

# Copy to dist/standalone
$dest = Join-Path $outDir 'arkan_poseidon.exe'
Copy-Item $exe $dest -Force

Write-Host ''
Write-Host 'Standalone ready:'
Write-Host $dest
