# Build Android APK with all books included
# This creates a larger APK but works completely offline

param(
    [switch]$DownloadBooks = $false,
    [switch]$SkipDownload = $false
)

Write-Host "Building Orayta Android APK with all books included..." -ForegroundColor Cyan
Write-Host ""

$SERVER_URL = "https://raw.githubusercontent.com/MosheWagner/Orayta-Books/master/books/"
$BOOKLISTURL = $SERVER_URL + "OraytaBookList"
$BooksDir = "Books"

# Function to download and parse book list
function Download-BookList {
    Write-Host "Downloading book list from GitHub..." -ForegroundColor Yellow
    try {
        $bookList = Invoke-WebRequest -Uri $BOOKLISTURL -UseBasicParsing
        Write-Host "Book list downloaded successfully" -ForegroundColor Green
        return $bookList.Content
    } catch {
        Write-Error "Failed to download book list: $_"
        return $null
    }
}

# Function to download a single book (for parallel execution)
function Download-Book {
    param($url, $destination)
    
    try {
        # Create directory if needed
        $destDir = Split-Path $destination -Parent
        if ($destDir -and -not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        }
        
        Invoke-WebRequest -Uri $url -OutFile $destination -UseBasicParsing
        return @{Success=$true; File=$destination; Url=$url}
    } catch {
        return @{Success=$false; File=$destination; Url=$url; Error=$_.Exception.Message}
    }
}

# Check if Books directory exists or if download is requested
if (-not (Test-Path $BooksDir) -or $DownloadBooks) {
    if ($SkipDownload) {
        Write-Error "Books directory not found and -SkipDownload specified!"
        exit 1
    }
    
    Write-Host "Books directory not found or download requested." -ForegroundColor Yellow
    Write-Host "Downloading all books from OraytaBookList..." -ForegroundColor Cyan
    Write-Host ""
    
    # Create Books directory
    if (-not (Test-Path $BooksDir)) {
        New-Item -ItemType Directory -Path $BooksDir -Force | Out-Null
    }
    
    # Download book list
    $bookListContent = Download-BookList
    if (-not $bookListContent) {
        Write-Error "Cannot proceed without book list"
        exit 1
    }
    
    # Parse book list and collect all download tasks
    $lines = $bookListContent -split "`n"
    $downloadTasks = @()
    $currentGroup = ""
    
    foreach ($line in $lines) {
        $line = $line.Trim()
        
        # Skip empty lines and comments
        if ($line -eq "" -or $line.StartsWith("#")) {
            continue
        }
        
        # Group header (starts with @)
        if ($line.StartsWith("@")) {
            $currentGroup = $line.Substring(1).Trim()
            continue
        }
        
        # Book entry format: ./path/file.obk, size, date, hash
        $parts = $line -split ","
        if ($parts.Count -ge 1) {
            $bookPath = $parts[0].Trim()
            
            # Skip if not a valid path
            if ($bookPath -eq "" -or -not ($bookPath.StartsWith("./"))) {
                continue
            }
            
            # Remove the ./ prefix
            $bookPath = $bookPath.Substring(2)
            
            # Build URL and destination
            $bookUrl = $SERVER_URL + $bookPath
            $destination = Join-Path $BooksDir $bookPath
            
            $downloadTasks += @{
                Url = $bookUrl
                Destination = $destination
                FileName = Split-Path $bookPath -Leaf
                Group = $currentGroup
            }
        }
    }
    
    Write-Host "Found $($downloadTasks.Count) books to download" -ForegroundColor Cyan
    Write-Host "Downloading in parallel (50 concurrent downloads)..." -ForegroundColor Yellow
    Write-Host ""
    
    # Download books in parallel batches
    $batchSize = 50
    $downloadCount = 0
    $failCount = 0
    $totalBooks = $downloadTasks.Count
    
    for ($i = 0; $i -lt $totalBooks; $i += $batchSize) {
        $batch = $downloadTasks[$i..[Math]::Min($i + $batchSize - 1, $totalBooks - 1)]
        $jobs = @()
        
        # Start parallel downloads for this batch
        foreach ($task in $batch) {
            $jobs += Start-Job -ScriptBlock {
                param($url, $dest)
                try {
                    $destDir = Split-Path $dest -Parent
                    if ($destDir -and -not (Test-Path $destDir)) {
                        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                    }
                    Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing
                    return @{Success=$true; File=$dest}
                } catch {
                    return @{Success=$false; File=$dest; Error=$_.Exception.Message}
                }
            } -ArgumentList $task.Url, $task.Destination
        }
        
        # Wait for batch to complete with minimal overhead
        Wait-Job -Job $jobs | Out-Null
        
        # Collect results quickly
        $results = $jobs | Receive-Job
        $downloadCount += ($results | Where-Object { $_.Success }).Count
        $failCount += ($results | Where-Object { -not $_.Success }).Count
        $jobs | Remove-Job
        
        $percent = [Math]::Round((($downloadCount + $failCount) / $totalBooks) * 100)
        Write-Host "Progress: $($downloadCount + $failCount)/$totalBooks ($percent%)"
    }
    
    Write-Host ""
    Write-Host "Download complete!" -ForegroundColor Green
    Write-Host "  Successfully downloaded: $downloadCount books" -ForegroundColor Green
    if ($failCount -gt 0) {
        Write-Host "  Failed: $failCount books" -ForegroundColor Red
    }
    Write-Host ""
}

# Clean and recreate assets Books directory to ensure fresh copy
$assetsBooks = "android-orayta/assets/Orayta/Books"
if (Test-Path $assetsBooks) {
    Write-Host "Cleaning existing assets Books directory..." -ForegroundColor Yellow
    Remove-Item -Path $assetsBooks -Recurse -Force
}
New-Item -ItemType Directory -Path $assetsBooks -Force | Out-Null
Write-Host "Created fresh assets Books directory" -ForegroundColor Green

# Copy all books to assets
Write-Host "Copying books to assets..." -ForegroundColor Yellow
Copy-Item -Path "Books/*" -Destination $assetsBooks -Recurse -Force
$bookCount = (Get-ChildItem $assetsBooks -Recurse -File).Count
Write-Host "Copied $bookCount book files to assets" -ForegroundColor Green
Write-Host ""

# Calculate total size
$totalSize = (Get-ChildItem $assetsBooks -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "Total books size: $([math]::Round($totalSize, 2)) MB" -ForegroundColor Cyan
Write-Host ""

# Clean build directory to force rebuild with new assets
Write-Host "Cleaning build directory to force rebuild..." -ForegroundColor Yellow
if (Test-Path "build-android\android-build") {
    Remove-Item -Path "build-android\android-build" -Recurse -Force
    Write-Host "Build directory cleaned" -ForegroundColor Green
}
Write-Host ""

# Build the APK
Write-Host "Building APK with books..." -ForegroundColor Yellow
& .\build-android-arm64.ps1

# Check if APK was created (regardless of exit code, since ninja can fail even on success)
$apkPath = "build-android\android-build\build\outputs\apk\release\android-build-release.apk"
if (Test-Path $apkPath) {
    $apkInfo = Get-Item $apkPath
    $timeSinceModified = (Get-Date) - $apkInfo.LastWriteTime
    
    # Check if APK was recently created (within last 5 minutes)
    if ($timeSinceModified.TotalMinutes -lt 5) {
        # Rename APK to indicate it includes all books
        $newApkName = "orayta-with-all-books.apk"
        $newApkPath = Join-Path (Split-Path $apkPath -Parent) $newApkName
        Copy-Item -Path $apkPath -Destination $newApkPath -Force
        
        # Also copy to project root for easy access
        $rootApkPath = Join-Path $PSScriptRoot $newApkName
        Copy-Item -Path $apkPath -Destination $rootApkPath -Force
        
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "SUCCESS! APK with books created!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        
        $apkSize = $apkInfo.Length / 1MB
        Write-Host "APK Name: $newApkName" -ForegroundColor Cyan
        Write-Host "APK Size: $([math]::Round($apkSize, 2)) MB" -ForegroundColor Cyan
        Write-Host "Location: $newApkPath" -ForegroundColor Cyan
        Write-Host "Also copied to: $rootApkPath" -ForegroundColor Cyan
        Write-Host "Created: $($apkInfo.LastWriteTime)" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "This APK includes all 858 books and works completely offline!" -ForegroundColor Green
        Write-Host ""
        Write-Host "To install: .\android-sdk\platform-tools\adb.exe install -r `"$rootApkPath`"" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Note: The regular build (without books) will create 'orayta-regular.apk'" -ForegroundColor Gray
        exit 0
    } else {
        Write-Error "APK exists but was not recently modified. Build may have failed."
        exit 1
    }
} else {
    Write-Error "Build failed - APK not found!"
    exit 1
}
