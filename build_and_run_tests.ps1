# 1. Go to the build folder (create if it doesn't exist)
$buildDir = "build"
if (-Not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir
}
Set-Location $buildDir

# 2. Configure CMake with MinGW
Write-Host "Configuring CMake..."
cmake .. -G "MinGW Makefiles"

# 3. Build everything
Write-Host "Building..."
mingw32-make

# 4. Run all test executables in build/tests/
Write-Host "Running tests..."
Get-ChildItem -Recurse -Filter *.exe | ForEach-Object {
    Write-Host "Running $($_.FullName)"
    & $_.FullName
}
