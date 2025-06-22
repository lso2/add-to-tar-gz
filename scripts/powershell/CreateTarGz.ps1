# Add to tar.gz - PowerShell Version
# Creates tar.gz archives using 7-Zip

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Path
)

# Check if path exists
if (-not (Test-Path $Path)) {
    Write-Host "ERROR: Path does not exist: $Path" -ForegroundColor Red
    Start-Sleep -Seconds 3
    exit 1
}

# Get item info
$item = Get-Item $Path
$parentDir = $item.DirectoryName
if (-not $parentDir) { $parentDir = $item.Parent.FullName }
$baseName = $item.BaseName
if (-not $baseName) { $baseName = $item.Name }

# Output paths
$tarFile = Join-Path $parentDir "$baseName.tar"
$tarGzFile = Join-Path $parentDir "$baseName.tar.gz"

# Check if output exists
if (Test-Path $tarGzFile) {
    Write-Host "WARNING: $baseName.tar.gz already exists!" -ForegroundColor Yellow
    $response = Read-Host "Overwrite? (Y/N)"
    if ($response -ne 'Y') {
        Write-Host "Operation cancelled." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
        exit 0
    }
    Remove-Item $tarGzFile -Force
}

# Find 7-Zip
$sevenZip = @(
    "${env:ProgramFiles}\7-Zip\7z.exe",
    "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $sevenZip) {
    Write-Host "ERROR: 7-Zip not found. Please install from https://www.7-zip.org/" -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}

# Show progress
Write-Host "Creating tar.gz archive..." -ForegroundColor Cyan
Write-Host "Source: $($item.Name)" -ForegroundColor Gray
Write-Host "Target: $baseName.tar.gz" -ForegroundColor Gray
Write-Host ""

# Create TAR
& $sevenZip a -ttar $tarFile $Path -bso0 -bsp0
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to create TAR archive." -ForegroundColor Red
    Start-Sleep -Seconds 3
    exit 1
}

# Compress to GZ
& $sevenZip a -tgzip $tarGzFile $tarFile -mx9 -bso0 -bsp0
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to compress to tar.gz." -ForegroundColor Red
    Remove-Item $tarFile -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 3
    exit 1
}

# Cleanup
Remove-Item $tarFile -Force -ErrorAction SilentlyContinue

# Get file size
$fileInfo = Get-Item $tarGzFile
$sizeMB = [math]::Round($fileInfo.Length / 1MB, 2)
$sizeKB = [math]::Round($fileInfo.Length / 1KB, 0)

# Success message
Write-Host ""
Write-Host "SUCCESS: Archive created!" -ForegroundColor Green
Write-Host "File: $baseName.tar.gz" -ForegroundColor White
if ($sizeMB -gt 1) {
    Write-Host "Size: $sizeMB MB" -ForegroundColor Gray
} else {
    Write-Host "Size: $sizeKB KB" -ForegroundColor Gray
}
Write-Host ""

Start-Sleep -Seconds 2
