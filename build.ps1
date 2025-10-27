# Build the project from scratch automatically with MinGW

$buildDir = Join-Path $PSScriptRoot "build"

# Optional: clean build
if (Test-Path $buildDir) {
    Write-Host "Removing existing build directory..."
    Remove-Item -Recurse -Force $buildDir
}

# Create build directory
Write-Host "Creating build directory..."
New-Item -ItemType Directory -Path $buildDir | Out-Null

# Go to build directory
Set-Location $buildDir

# Configure CMake using MinGW
Write-Host "Configuring CMake..."
cmake -G "MinGW Makefiles" $PSScriptRoot

# Build project
Write-Host "Building project..."
cmake --build . --config Release

# Back to root folder
Set-Location $PSScriptRoot

Write-Host "Build complete!"


# Run main_exe
#$exePath = "$buildDir/main_exe.exe"
#if (Test-Path $exePath) {
#    Write-Host "Running main_exe..."
#    & $exePath
#}