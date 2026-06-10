# PowerShell Test Runner for Canopus Modbus App

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$MocksDir = Join-Path $ScriptDir "mocks"
$ArduinoJsonHeader = Join-Path $MocksDir "ArduinoJson.h"

# 1. Check and download ArduinoJson.h if missing or invalid
if (Test-Path $ArduinoJsonHeader) {
    $FileSize = (Get-Item $ArduinoJsonHeader).Length
    if ($FileSize -lt 10000) {
        Write-Host "Existing ArduinoJson.h is too small ($FileSize bytes). Deleting to re-download correct single-header..." -ForegroundColor Yellow
        Remove-Item $ArduinoJsonHeader -Force
    }
}

if (-not (Test-Path $ArduinoJsonHeader)) {
    Write-Host "ArduinoJson.h not found or invalid in mocks directory. Downloading single-header from GitHub Releases..." -ForegroundColor Cyan
    $Url = "https://github.com/bblanchon/ArduinoJson/releases/download/v6.21.3/ArduinoJson-v6.21.3.h"
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $Url -OutFile $ArduinoJsonHeader -UseBasicParsing
        Write-Host "ArduinoJson.h single-header downloaded successfully!" -ForegroundColor Green
    } catch {
        Write-Error "Failed to download ArduinoJson.h from $Url. Please download it manually and place it in tests/mocks/."
        exit 1
    }
}

# Try to find g++ if it is not in the system PATH
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Host "g++ not found in PATH. Searching local winget packages or common paths..." -ForegroundColor Yellow
    $PathsToSearch = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages",
        "C:\msys64\mingw64\bin",
        "C:\MinGW\bin"
    )
    foreach ($P in $PathsToSearch) {
        if (Test-Path $P) {
            $GppExe = Get-ChildItem -Path $P -Filter "g++.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($GppExe) {
                $BinDir = Split-Path -Parent $GppExe.FullName
                $env:PATH = "$BinDir;$env:PATH"
                Write-Host "Found g++ compiler at: $BinDir. Temporarily added to PATH." -ForegroundColor Green
                break
            }
        }
    }
}

# 2. Compile tests using g++
Write-Host "Compiling test runner..." -ForegroundColor Cyan

$SourceFiles = @(
    "tests/run_tests.cpp",
    "tests/test_crc16.cpp",
    "tests/test_modbus.cpp",
    "tests/test_settings.cpp",
    "CanopusModbus.cpp",
    "Settings.cpp"
)

$CompileCmd = "g++ -std=c++11 -I. -Itests/mocks -o tests/run_tests.exe " + ($SourceFiles -join " ")

Write-Host "Executing build command: $CompileCmd" -ForegroundColor Gray

Invoke-Expression $CompileCmd

if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed. Ensure g++ (GCC) is installed and in your PATH. If you just installed it, please restart the terminal or IDE."
    exit 1
}

Write-Host "Compilation successful!" -ForegroundColor Green

# 3. Run the compiled executable
Write-Host "Executing unit tests..." -ForegroundColor Cyan
Write-Host "-------------------------------------------" -ForegroundColor Gray

$TestRunnerExe = Join-Path $ScriptDir "run_tests.exe"
Invoke-Expression $TestRunnerExe

if ($LASTEXITCODE -ne 0) {
    Write-Error "Some unit tests failed."
    exit $LASTEXITCODE
}

Write-Host "-------------------------------------------" -ForegroundColor Gray
Write-Host "Test execution completed successfully!" -ForegroundColor Green
