# Extract tar.gz - PowerShell Version
# Extracts tar.gz and tgz archives using 7-Zip

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Path
)

# Check if file exists
if (-not (Test-Path $Path)) {
    Write-Host "ERROR: File does not exist: $Path" -ForegroundColor Red
    Start-Sleep -Seconds 3
    exit 1
}

# Get file info
$file = Get-Item $Path
$directory = $file.DirectoryName
$fileName = $file.Name

# Check extension
$validExtensions = @('.tar.gz', '.tgz')
$isValid = $false
foreach ($ext in $validExtensions) {
    if ($fileName.EndsWith($ext, [StringComparison]::OrdinalIgnoreCase)) {
        $isValid = $true
        break
    }
}

if (-not $isValid) {
    Write-Host "ERROR: This script only extracts .tar.gz or .tgz files." -ForegroundColor Red
    Write-Host "File: $fileName" -ForegroundColor Gray
    Start-Sleep -Seconds 5
    exit 1
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

# Determine extraction folder
if ($fileName.EndsWith('.tar.gz', [StringComparison]::OrdinalIgnoreCase)) {
    $extractFolder = Join-Path $directory ($file.BaseName -replace '\.tar$', '')
} else {
    $extractFolder = Join-Path $directory $file.BaseName
}

# Extract
Write-Host "Extracting $fileName..." -ForegroundColor Cyan
Write-Host ""

# Create temp directory
$tempDir = Join-Path $env:TEMP "extract_$(Get-Random)"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

try {
    # Extract gzip
    & $sevenZip e -y -o"$tempDir" $Path *.tar 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to extract gzip archive"
    }
    
    # Find tar file
    $tarFile = Get-ChildItem -Path $tempDir -Filter "*.tar" | Select-Object -First 1
    if (-not $tarFile) {
        throw "No tar file found after extraction"
    }
    
    # Extract tar
    & $sevenZip x -y -o"$extractFolder" $tarFile.FullName 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to extract tar archive"
    }
    
    Write-Host ""
    Write-Host "Successfully extracted to: $extractFolder" -ForegroundColor Green
    Write-Host ""
    Start-Sleep -Seconds 2
}
catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    Start-Sleep -Seconds 5
    exit 1
}
finally {
    # Cleanup
    Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}
