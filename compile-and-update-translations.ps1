# Compile and deploy translation files
# This script:
# 1. Updates .ts files from source code (lupdate)
# 2. Compiles .ts files to .qm files (lrelease)
# 3. Copies them to the Android assets folder

Write-Host "=== Qt Translation Update and Compilation ===" -ForegroundColor Cyan
Write-Host ""

# Check if Qt tools are available
$lupdatePath = Get-Command lupdate -ErrorAction SilentlyContinue
$lreleasePath = Get-Command lrelease -ErrorAction SilentlyContinue

if (-not $lupdatePath -or -not $lreleasePath) {
    Write-Host "⚠ Qt translation tools not found in PATH" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Searching for Qt installation..." -ForegroundColor Cyan
    
    # Common Qt installation paths
    $qtPaths = @(
        "C:\Qt\6.10.0\msvc2022_64\bin",
        "C:\Qt\6.9.0\msvc2022_64\bin",
        "C:\Qt\6.8.0\msvc2022_64\bin",
        "C:\Qt\6.7.0\msvc2022_64\bin"
    )
    
    $foundPath = $null
    foreach ($path in $qtPaths) {
        if (Test-Path "$path\lupdate.exe") {
            $foundPath = $path
            break
        }
    }
    
    if ($foundPath) {
        Write-Host "✓ Found Qt tools at: $foundPath" -ForegroundColor Green
        Write-Host "Adding to PATH for this session..." -ForegroundColor Yellow
        $env:PATH = "$foundPath;$env:PATH"
    } else {
        Write-Host "✗ Could not find Qt tools automatically" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please add Qt bin directory to your PATH:" -ForegroundColor Yellow
        Write-Host "  Example: C:\Qt\6.10.0\msvc2022_64\bin" -ForegroundColor White
        Write-Host ""
        Write-Host "Or run this command before running the script:" -ForegroundColor Yellow
        Write-Host '  $env:PATH = "C:\Qt\6.10.0\msvc2022_64\bin;$env:PATH"' -ForegroundColor White
        Write-Host ""
        exit 1
    }
}

Write-Host ""

# Step 1: Update translation files from source code
Write-Host "Step 1: Updating translation files from source code..." -ForegroundColor Cyan
Write-Host ""

Write-Host "Running lupdate to extract translatable strings..." -ForegroundColor Yellow

# Collect all source files
$sourceFiles = @()
$sourceFiles += Get-ChildItem -Path "Mobile" -Filter "*.cpp" -File | ForEach-Object { $_.FullName }
$sourceFiles += Get-ChildItem -Path "Mobile" -Filter "*.h" -File | ForEach-Object { $_.FullName }
$sourceFiles += Get-ChildItem -Path "Mobile" -Filter "*.ui" -File | ForEach-Object { $_.FullName }
$sourceFiles += Get-ChildItem -Path "OraytaBase" -Filter "*.cpp" -File -Recurse | ForEach-Object { $_.FullName }
$sourceFiles += Get-ChildItem -Path "OraytaBase" -Filter "*.h" -File -Recurse | ForEach-Object { $_.FullName }
$sourceFiles += "main.cpp"

Write-Host "Found $($sourceFiles.Count) source files to scan" -ForegroundColor Gray

# Update Hebrew translations
Write-Host "Updating Hebrew.ts..." -ForegroundColor Yellow
& lupdate $sourceFiles -ts Hebrew.ts
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Hebrew.ts updated successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Hebrew.ts update failed" -ForegroundColor Red
    Write-Host "Make sure Qt tools (lupdate) are in your PATH" -ForegroundColor Yellow
    exit 1
}

# Update French translations
Write-Host "Updating French.ts..." -ForegroundColor Yellow
& lupdate $sourceFiles -ts French.ts
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ French.ts updated successfully" -ForegroundColor Green
} else {
    Write-Host "✗ French.ts update failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Step 2: Compiling translation files..." -ForegroundColor Cyan
Write-Host ""

# Compile Hebrew
Write-Host "Compiling Hebrew.ts..." -ForegroundColor Yellow
lrelease Hebrew.ts -qm Hebrew.qm
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Hebrew compiled successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Hebrew compilation failed" -ForegroundColor Red
    exit 1
}

# Compile French
Write-Host "Compiling French.ts..." -ForegroundColor Yellow
lrelease French.ts -qm French.qm
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ French compiled successfully" -ForegroundColor Green
} else {
    Write-Host "✗ French compilation failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Copying to Android assets..." -ForegroundColor Cyan

# Copy to Android assets
Copy-Item Hebrew.qm android-orayta/assets/Orayta/Hebrew.qm -Force
Write-Host "✓ Hebrew.qm copied to android-orayta/assets/Orayta/" -ForegroundColor Green

Copy-Item French.qm android-orayta/assets/Orayta/French.qm -Force
Write-Host "✓ French.qm copied to android-orayta/assets/Orayta/" -ForegroundColor Green

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "✓ Translation files updated from source code (lupdate)" -ForegroundColor Green
Write-Host "✓ Translation files compiled to .qm format (lrelease)" -ForegroundColor Green
Write-Host "✓ Compiled files deployed to Android assets" -ForegroundColor Green
Write-Host ""

# Show file info
Write-Host "Deployed translation files:" -ForegroundColor Cyan
Get-ChildItem android-orayta/assets/Orayta/*.qm | Select-Object Name, @{Name="Size (KB)";Expression={[math]::Round($_.Length/1KB, 2)}}, LastWriteTime | Format-Table -AutoSize

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Review the updated .ts files (Hebrew.ts, French.ts)" -ForegroundColor White
Write-Host "2. Add translations for any new <translation type='unfinished'> entries" -ForegroundColor White
Write-Host "3. Run this script again after adding translations" -ForegroundColor White
Write-Host "4. Rebuild the APK to include the updated translations" -ForegroundColor White
Write-Host ""
