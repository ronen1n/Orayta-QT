# PowerShell script to build Android APK for Qt6 Orayta (ARM64 only for real devices)

# Set Android SDK and NDK paths
$env:ANDROID_SDK_ROOT = "$PSScriptRoot\android-sdk"
$env:ANDROID_NDK_ROOT = "$env:ANDROID_SDK_ROOT\ndk\27.2.12479018"
$env:ANDROID_HOME = $env:ANDROID_SDK_ROOT

# Set Qt6 paths
$env:QT_HOST_PATH = "C:\Qt\6.10.0\msvc2022_64"
$env:QT_ANDROID_PATH = "C:\Qt\6.10.0\android_arm64_v8a"
$env:JAVA_HOME = "C:\Program Files\BellSoft\LibericaJDK-17-Full"

# Add tools to PATH
$env:PATH = "$env:ANDROID_SDK_ROOT\platform-tools;$env:ANDROID_SDK_ROOT\build-tools\35.0.0;$env:QT_HOST_PATH\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;$env:PATH"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building ARM64 Android APK (for real devices)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Environment configured:"
Write-Host "Android SDK Root: $env:ANDROID_SDK_ROOT"
Write-Host "Android NDK Root: $env:ANDROID_NDK_ROOT"
Write-Host "Qt Host Path: $env:QT_HOST_PATH"
Write-Host "Qt Android Path: $env:QT_ANDROID_PATH"
Write-Host "Java Home: $env:JAVA_HOME"
Write-Host ""

# Clean previous build
$buildDir = "bld-arm64"
if (Test-Path $buildDir) {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    Remove-Item -Path $buildDir -Recurse -Force
}

# Configure CMake
Write-Host "Configuring CMake for ARM64 only..." -ForegroundColor Cyan
cmake -B $buildDir -S . -G "Ninja" `
    -DCMAKE_TOOLCHAIN_FILE="$env:ANDROID_NDK_ROOT\build\cmake\android.toolchain.cmake" `
    -DANDROID_ABI=arm64-v8a `
    -DANDROID_PLATFORM=android-28 `
    -DANDROID_STL=c++_shared `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" `
    -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG" `
    -DQT_HOST_PATH="$env:QT_HOST_PATH" `
    -DCMAKE_PREFIX_PATH="$env:QT_ANDROID_PATH" `
    -DCMAKE_FIND_ROOT_PATH="$env:QT_ANDROID_PATH" `
    -DANDROID_SDK_ROOT="$env:ANDROID_SDK_ROOT" `
    -DQT_ANDROID_BUILD_ALL_ABIS=OFF `
    -DQT_ANDROID_ABIS="arm64-v8a"

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build the APK
Write-Host ""
Write-Host "Building ARM64 APK..." -ForegroundColor Cyan
$env:ANDROID_BUILD_TOOLS_REVISION = "33.0.2"
cmake --build $buildDir --target apk

# Check if APK was created
$apkPath = "$buildDir\android-build\build\outputs\apk\release\android-build-release.apk"
if (Test-Path $apkPath) {
    $apkInfo = Get-Item $apkPath
    
    # Rename APK
    $newApkName = "orayta-arm64.apk"
    $newApkPath = Join-Path (Split-Path $apkPath -Parent) $newApkName
    Copy-Item -Path $apkPath -Destination $newApkPath -Force
    
    # Copy to project root
    $rootApkPath = Join-Path $PSScriptRoot $newApkName
    Copy-Item -Path $apkPath -Destination $rootApkPath -Force
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "APK Name: $newApkName" -ForegroundColor Green
    Write-Host "Size: $([math]::Round($apkInfo.Length / 1MB, 2)) MB" -ForegroundColor Green
    Write-Host "Location: $newApkPath" -ForegroundColor Green
    Write-Host "Also copied to: $rootApkPath" -ForegroundColor Green
    Write-Host "Supported ABI: arm64-v8a (real devices only)" -ForegroundColor Green
    Write-Host ""
    Write-Host "This APK downloads books on-demand (requires internet)" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To install on real device: .\android-sdk\platform-tools\adb.exe install -r `"$rootApkPath`"" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Note: This will NOT work on x86/x86_64 emulators!" -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "Build failed - APK not found!" -ForegroundColor Red
    exit 1
}
