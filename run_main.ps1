# Quickly compile and run main.cpp (works even if run repeatedly)

$root = $PSScriptRoot
$src = Join-Path $root "src\main.cpp"
$buildDir = Join-Path $root "build"
$outExe = Join-Path $buildDir "main_exe.exe"

$includeDirs = @(
    (Join-Path $root "include"),
    (Join-Path $root "external\curl\include")
)
$libDirs = @(
    (Join-Path $root "external\curl\lib")
)
$dllDir = Join-Path $root "external\curl\bin"

# Ensure build folder exists
if (-Not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Build argument array for g++
$args = @()
$args += $src
foreach ($inc in $includeDirs) { $args += "-I$inc" }
foreach ($ld in $libDirs)      { $args += "-L$ld"  }

# libraries to link (use import .dll.a files in lib dir)
$args += "-lcurl"
$args += "-lssl"
$args += "-lcrypto"
$args += "-lz"

# output file
$args += "-o"
$args += $outExe

Write-Host "Compiling main.cpp..."
Write-Host "g++ " + ($args -join ' ')
# Run g++ with the argument array (avoids quoting/Out-String issues)
& g++ @args

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed (exit code $LASTEXITCODE). Not running executable." -ForegroundColor Red
    exit $LASTEXITCODE
}

# Add DLL folder to PATH for this session so libcurl DLL is found at runtime
$env:PATH = "$dllDir;$env:PATH"

Write-Host "Running main_exe.exe..."
& $outExe
