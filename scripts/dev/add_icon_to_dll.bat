@echo off
echo Adding icon to DLL context menu...

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Must run as Administrator
    pause
    exit /b 1
)

:: Get the icon path
set "ICON_PATH=%ProgramData%\AddToTarGz\tgz.ico"

:: Find the DLL's CLSID in registry and add icon to it
for /f "tokens=*" %%a in ('reg query "HKCR\*\shellex\ContextMenuHandlers\TarGzExtract" /ve 2^>nul ^| find "REG_SZ"') do (
    for /f "tokens=3" %%b in ("%%a") do (
        set "CLSID=%%b"
        echo Found CLSID: %%b
        
        :: Add icon to the CLSID
        reg add "HKCR\CLSID\%%b" /v "Icon" /t REG_SZ /d "%ICON_PATH%" /f >nul
        echo Icon added to DLL context menu
    )
)

echo Done! The DLL menu should now show the icon.
pause
