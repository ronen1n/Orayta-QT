# Build Orayta for ARMv7 (32-bit ARM) - for older Android devices
# This is for devices that show "Package parsing error" with ARM64 builds

# Set Android SDK and NDK paths (same as ARM64 build)
$env:ANDROID_SDK_ROOT = "$PSScriptRoot\android-sdk"
$env:ANDROID_NDK_ROOT = "$env:ANDROID_SDK_ROOT\ndk\27.2.12479018"
$env:ANDROID_HOME = $env:ANDROID_SDK_ROOT

# Set Qt6 paths for ARMv7
$env:QT_HOST_PATH = "C:\Qt\6.10.0\msvc2022_64"
$env:QT_ANDROID_PATH = "C:\Qt\6.10.0\android_arm64_v8a"
$env:JAVA_HOME = "C:\Program Files\BellSoft\LibericaJDK-17-Full"

# Check if Qt for ARMv7 is installed
if (-not (Test-Path $env:QT_ANDROID_PATH)) {
    Write-Error "Qt for Android ARMv7 not found at: $env:QT_ANDROID_PATH"
    Write-Host ""
    Write-Host "Please install Qt for Android ARMv7 using Qt Maintenance Tool:" -ForegroundColor Yellow
    Write-Host "1. Run Qt Maintenance Tool" -ForegroundColor Yellow
    Write-Host "2. Select 'Add or remove components'" -ForegroundColor Yellow
    Write-Host "3. Under Qt 6.5.3, check 'Android ARMv7'" -ForegroundColor Yellow
    Write-Host "4. Install and run this script again" -ForegroundColor Yellow
    exit 1
}

# Add tools to PATH
$env:PATH = "$env:ANDROID_SDK_ROOT\platform-tools;$env:ANDROID_SDK_ROOT\build-tools\35.0.0;$env:QT_HOST_PATH\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;$env:PATH"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building ARMv7 Android APK (for older devices)" -ForegroundColor Cyan
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
$buildDir = "bld-armv7"
if (Test-Path $buildDir) {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    Remove-Item -Path $buildDir -Recurse -Force
}

# Configure CMake
Write-Host "Configuring CMake for ARMv7..." -ForegroundColor Cyan
cmake -B $buildDir -S . -G "Ninja" `
    -DCMAKE_TOOLCHAIN_FILE="$env:ANDROID_NDK_ROOT\build\cmake\android.toolchain.cmake" `
    -DANDROID_ABI=armeabi-v7a `
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
    -DQT_ANDROID_ABIS="armeabi-v7a"

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build the APK
Write-Host ""
Write-Host "Building ARMv7 APK..." -ForegroundColor Cyan
cmake --build $buildDir --target apk

# Check if APK was created
$apkPath = "$buildDir\android-build\build\outputs\apk\release\android-build-release.apk"
if (Test-Path $apkPath) {
    $apkInfo = Get-Item $apkPath
    
    # Rename APK
    $newApkName = "orayta-armv7.apk"
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
    Write-Host "Supported ABI: armeabi-v7a (32-bit ARM - older devices)" -ForegroundColor Green
    Write-Host ""
    Write-Host "This APK works on older Android devices (Android 7.0+)" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To install: .\android-sdk\platform-tools\adb.exe install -r `"$rootApkPath`"" -ForegroundColor Yellow
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "Build failed - APK not found!" -ForegroundColor Red
    exit 1
}
