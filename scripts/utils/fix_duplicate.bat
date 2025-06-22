@echo off
echo Removing duplicate registry menu for .tar.gz files...

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Must run as Administrator
    pause
    exit /b 1
)

:: Remove the registry-based extraction menu for .tar.gz files only
:: (Keep it for .tgz files since DLL might not catch those)
reg delete "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /f >nul 2>&1

echo Duplicate menu removed for .tar.gz files.
echo The DLL will handle .tar.gz files with dynamic names.
echo Registry will still handle .tgz files.
pause
