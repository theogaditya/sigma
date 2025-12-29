#Requires -Version 5.1
<#
.SYNOPSIS
    Sigma Language Installer for Windows
.DESCRIPTION
    Downloads and installs the Sigma programming language compiler.
    One-line install: irm https://raw.githubusercontent.com/YOUR_USERNAME/sigma-lang/main/packaging/install.ps1 | iex
.NOTES
    Run in PowerShell as Administrator for system-wide install,
    or as regular user for user-only install.
#>

$ErrorActionPreference = "Stop"

# Configuration
$Repo = "YOUR_USERNAME/sigma-lang"  # TODO: Update with actual repo
$BinaryName = "sigma.exe"

# Colors and formatting
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Banner {
    Write-Host ""
    Write-Host "  ╔═══════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "  ║     Sigma Language Installer  🔥      ║" -ForegroundColor Cyan
    Write-Host "  ╚═══════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
}

function Get-InstallDirectory {
    # Check if running as admin
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    
    if ($isAdmin) {
        $installDir = "$env:ProgramFiles\Sigma"
    } else {
        $installDir = "$env:LOCALAPPDATA\Sigma"
    }
    
    return $installDir
}

function Get-LatestVersion {
    Write-Host "→ Fetching latest version..." -ForegroundColor Cyan
    
    try {
        $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest"
        $version = $releases.tag_name
        Write-Host "✓ Latest version: $version" -ForegroundColor Green
        return $version
    } catch {
        Write-Host "Warning: Could not fetch latest version. Using 'latest'." -ForegroundColor Yellow
        return "latest"
    }
}

function Download-Binary {
    param($Version, $TempDir)
    
    $binary = "sigma-windows-x86_64.exe"
    $downloadUrl = "https://github.com/$Repo/releases/download/$Version/$binary"
    $tempFile = Join-Path $TempDir $BinaryName
    
    Write-Host "→ Downloading $binary..." -ForegroundColor Cyan
    
    try {
        # Use TLS 1.2
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $downloadUrl -OutFile $tempFile -UseBasicParsing
        Write-Host "✓ Downloaded successfully" -ForegroundColor Green
        return $tempFile
    } catch {
        Write-Host "Error: Failed to download binary." -ForegroundColor Red
        Write-Host "URL: $downloadUrl" -ForegroundColor Yellow
        throw
    }
}

function Install-Binary {
    param($TempFile, $InstallDir)
    
    Write-Host "→ Installing to $InstallDir..." -ForegroundColor Cyan
    
    # Create install directory if it doesn't exist
    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }
    
    # Copy binary
    $destPath = Join-Path $InstallDir $BinaryName
    Copy-Item -Path $TempFile -Destination $destPath -Force
    
    Write-Host "✓ Installed to $destPath" -ForegroundColor Green
    return $InstallDir
}

function Add-ToPath {
    param($InstallDir)
    
    Write-Host "→ Updating PATH..." -ForegroundColor Cyan
    
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    
    if ($isAdmin) {
        $pathType = "Machine"
        $currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
    } else {
        $pathType = "User"
        $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    }
    
    if ($currentPath -notlike "*$InstallDir*") {
        $newPath = "$currentPath;$InstallDir"
        [Environment]::SetEnvironmentVariable("Path", $newPath, $pathType)
        
        # Also update current session
        $env:Path = "$env:Path;$InstallDir"
        
        Write-Host "✓ Added to $pathType PATH" -ForegroundColor Green
    } else {
        Write-Host "✓ Already in PATH" -ForegroundColor Green
    }
}

function Test-Installation {
    param($InstallDir)
    
    $sigmaPath = Join-Path $InstallDir $BinaryName
    
    if (Test-Path $sigmaPath) {
        Write-Host ""
        Write-Host "✓ Sigma installed successfully!" -ForegroundColor Green
        Write-Host ""
        
        # Try to run --version
        try {
            & $sigmaPath --version
        } catch {
            Write-Host "Sigma version: (run 'sigma --version' to verify)" -ForegroundColor Yellow
        }
        
        Write-Host ""
        Write-Host "Get started:" -ForegroundColor Cyan
        Write-Host "  sigma --help          # Show help"
        Write-Host "  sigma program.sigma   # Run a program"
        Write-Host "  sigma                 # Start REPL"
        Write-Host ""
        Write-Host "Note: You may need to restart your terminal for PATH changes to take effect." -ForegroundColor Yellow
    } else {
        Write-Host "Error: Installation verification failed." -ForegroundColor Red
    }
}

# Main installation
function Main {
    Write-Banner
    
    $installDir = Get-InstallDirectory
    $version = Get-LatestVersion
    
    # Create temp directory
    $tempDir = Join-Path $env:TEMP "sigma-install-$(Get-Random)"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    
    try {
        $tempFile = Download-Binary -Version $version -TempDir $tempDir
        $installed = Install-Binary -TempFile $tempFile -InstallDir $installDir
        Add-ToPath -InstallDir $installDir
        Test-Installation -InstallDir $installDir
    } finally {
        # Cleanup
        if (Test-Path $tempDir) {
            Remove-Item -Path $tempDir -Recurse -Force
        }
    }
}

Main
