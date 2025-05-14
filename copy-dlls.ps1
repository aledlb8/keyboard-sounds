# PowerShell script to copy SFML DLLs to the build directory
$ErrorActionPreference = "Stop"

Write-Host "Copying SFML DLLs to build directory..." -ForegroundColor Cyan

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "Error: Build directory not found. Please build the project first." -ForegroundColor Red
    exit 1
}

# Check if SFML directory exists
$sfmlPath = Join-Path $PSScriptRoot "libs\SFML"
if (-not (Test-Path $sfmlPath)) {
    Write-Host "Error: SFML directory not found at $sfmlPath" -ForegroundColor Red
    exit 1
}

# Check if SFML bin directory exists
$sfmlBinPath = Join-Path $sfmlPath "bin"
if (-not (Test-Path $sfmlBinPath)) {
    Write-Host "Error: SFML bin directory not found at $sfmlBinPath" -ForegroundColor Red
    exit 1
}

# Define DLLs to copy
$dllsToCopy = @(
    "sfml-audio-3.dll",
    "sfml-system-3.dll"
)

# Debug DLLs if needed
$debugDlls = @(
    "sfml-audio-d-3.dll",
    "sfml-system-d-3.dll"
)

# Copy release DLLs
foreach ($dll in $dllsToCopy) {
    $sourcePath = Join-Path $sfmlBinPath $dll
    $destPath = Join-Path "build" $dll
    
    if (Test-Path $sourcePath) {
        Copy-Item -Path $sourcePath -Destination $destPath -Force
        Write-Host "Copied $dll to build directory" -ForegroundColor Green
    } else {
        Write-Host "Warning: $dll not found at $sourcePath" -ForegroundColor Yellow
    }
}

# Ask if debug DLLs should be copied
$copyDebug = Read-Host "Do you want to copy debug DLLs as well? (y/n)"
if ($copyDebug -eq "y" -or $copyDebug -eq "Y") {
    foreach ($dll in $debugDlls) {
        $sourcePath = Join-Path $sfmlBinPath $dll
        $destPath = Join-Path "build" $dll
        
        if (Test-Path $sourcePath) {
            Copy-Item -Path $sourcePath -Destination $destPath -Force
            Write-Host "Copied $dll to build directory" -ForegroundColor Green
        } else {
            Write-Host "Warning: $dll not found at $sourcePath" -ForegroundColor Yellow
        }
    }
}

Write-Host "Done!" -ForegroundColor Cyan 