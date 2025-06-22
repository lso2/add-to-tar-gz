# PowerShell Version

This folder contains PowerShell versions of the compression and extraction scripts.

## Features

- Same functionality as batch files
- Better error handling
- Cleaner code structure
- Cross-platform potential (with PowerShell Core)

## Usage

### Direct Usage
```powershell
# Compress
.\CreateTarGz.ps1 "C:\Path\To\File-or-Folder"

# Extract
.\ExtractTarGz.ps1 "C:\Path\To\archive.tar.gz"
```

### Context Menu Integration
To use PowerShell scripts in the context menu, modify the registry files to use:
```
powershell.exe -ExecutionPolicy Bypass -File "C:\Path\To\CreateTarGz.ps1" "%1"
```

## Requirements

- PowerShell 5.0 or higher (included in Windows 10)
- 7-Zip installed

## Notes

- PowerShell scripts may require execution policy changes
- Slightly slower startup than batch files
- Better for complex operations and error handling
