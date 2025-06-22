param(
    [Parameter(Mandatory=$true)]
    [string]$FilePath
)

# This script dynamically creates context menu entries and handles extraction

Add-Type -AssemblyName System.Windows.Forms

# Get filename and determine extraction folder name
$fileName = Split-Path -Leaf $FilePath
$parentDir = Split-Path -Parent $FilePath

if ($fileName -match '\.tar\.gz$') {
    $folderName = $fileName -replace '\.tar\.gz$', ''
} elseif ($fileName -match '\.tgz$') {
    $folderName = $fileName -replace '\.tgz$', ''
} else {
    [System.Windows.Forms.MessageBox]::Show("Unsupported file type", "Error")
    exit 1
}

# Get the directory containing this script and call the extraction batch file
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$extractScript = Join-Path $scriptDir "ExtractTarGz.bat"

# Show what we're doing (optional - remove for silent operation)
# [System.Windows.Forms.MessageBox]::Show("Extracting $fileName to $folderName/", "Extracting...")

# Call the actual extraction
$process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"$extractScript`" `"$FilePath`"" -WindowStyle Hidden -Wait -PassThru

if ($process.ExitCode -eq 0) {
    # Success - could show notification but keeping it silent
    # [System.Windows.Forms.MessageBox]::Show("Extracted to $folderName/", "Complete")
} else {
    [System.Windows.Forms.MessageBox]::Show("Extraction failed", "Error")
}
