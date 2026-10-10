[CmdletBinding()]
param(
    [string]$Config = "Debug",
    [string]$Preset = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Git is not installed or not in PATH." -ForegroundColor Red
    Write-Host "Install Git with the following command:" -ForegroundColor Yellow
    Write-Host "  winget install --id Git.Git -e --source winget" -ForegroundColor Yellow
    exit 1
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        Write-Host "CMake was not found in PATH." -ForegroundColor Red
        Write-Host "Visual Studio installer was not found." -ForegroundColor Red
        Write-Host "Install Visual Studio 2022 Community with the C++ workload:" -ForegroundColor Yellow
        Write-Host "  winget install --id Microsoft.VisualStudio.2022.Community --override `"--passive --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended`"" -ForegroundColor Yellow
        Write-Host "Alternatively, install CMake directly:" -ForegroundColor Yellow
        Write-Host "  winget install --id Kitware.CMake -e --source winget" -ForegroundColor Yellow
        exit 1
    }

    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) {
        $vsPath = & $vswhere -latest -property installationPath
    }

    if (-not $vsPath) {
        Write-Host "Visual Studio was not found." -ForegroundColor Red
        Write-Host "Install Visual Studio 2022 Community with the C++ workload:" -ForegroundColor Yellow
        Write-Host "  winget install --id Microsoft.VisualStudio.2022.Community --override `"--passive --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended`"" -ForegroundColor Yellow
        exit 1
    }

    $launchScript = Join-Path $vsPath "Common7\Tools\Launch-VsDevShell.ps1"
    if (-not (Test-Path $launchScript)) {
        Write-Host "Launch-VsDevShell.ps1 was not found in Visual Studio." -ForegroundColor Red
        Write-Host "Reinstall or repair Visual Studio 2022." -ForegroundColor Yellow
        exit 1
    }

    Write-Host "Loading Visual Studio developer environment." -ForegroundColor Cyan
    & $launchScript -Arch amd64 -SkipAutomaticLocation

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Host "CMake was not found in the Visual Studio environment." -ForegroundColor Red
        Write-Host "Install CMake with winget:" -ForegroundColor Yellow
        Write-Host "  winget install --id Kitware.CMake -e --source winget" -ForegroundColor Yellow
        exit 1
    }
}

$repoRoot = (Resolve-Path "$PSScriptRoot\..").Path

if ($Preset) {
    if ($Clean) {
        $presetFile = Join-Path $repoRoot "CMakePresets.json"
        if (Test-Path $presetFile) {
            $presetData = Get-Content $presetFile -Raw | ConvertFrom-Json
            $matched = $presetData.configurePresets | Where-Object { $_.name -eq $Preset }
            if ($matched -and $matched.binaryDir) {
                $targetDir = $matched.binaryDir.Replace('${sourceDir}', $repoRoot)
                if (Test-Path $targetDir) {
                    Write-Host "Cleaning $targetDir" -ForegroundColor Yellow
                    Remove-Item -Recurse -Force $targetDir
                }
            }
        }
    }

    Write-Host "Configuring with preset $Preset." -ForegroundColor Cyan
    cmake --preset $Preset -S $repoRoot
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Configuration failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }

    Write-Host "Building preset $Preset." -ForegroundColor Cyan
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    $buildDir = Join-Path $repoRoot "build"
    if ($Clean) {
        if (Test-Path $buildDir) {
            Write-Host "Cleaning $buildDir" -ForegroundColor Yellow
            Remove-Item -Recurse -Force $buildDir
        }
    }

    Write-Host "Configuring project." -ForegroundColor Cyan
    cmake -B $buildDir -S $repoRoot
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Configuration failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }

    Write-Host "Building project for $Config." -ForegroundColor Cyan
    cmake --build $buildDir --config $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "Build finished successfully." -ForegroundColor Green
