param(
    [Parameter(Mandatory=$true)]
    [string]$FilePath
)

# Get filename without path
$fileName = Split-Path -Leaf $FilePath

# Determine the extraction folder name
if ($fileName -match '\.tar\.gz$') {
    $folderName = $fileName -replace '\.tar\.gz$', ''
} elseif ($fileName -match '\.tgz$') {
    $folderName = $fileName -replace '\.tgz$', ''
} else {
    Write-Error "Unsupported file type"
    exit 1
}

# Get the directory containing the script
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$extractScript = Join-Path $scriptDir "ExtractTarGz.bat"

# Call the extraction script
& $extractScript $FilePath
