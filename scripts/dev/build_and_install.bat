@echo off
echo ================================================================
echo Building and Installing TarGz Context Menu DLL with Top Position
echo ================================================================
echo.

echo Step 1: Building DLL with menu position fix...
call "%~dp0build_dll.bat"
if errorlevel 1 (
    echo Build failed! Stopping.
    pause
    exit /b 1
)

echo.
echo Step 2: Installing the updated DLL...
echo.

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Must run as Administrator to install DLL
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Copy new DLL to system32
if exist "%~dp0..\dll\targz_context.dll" (
    echo Copying new DLL to System32...
    copy /Y "%~dp0..\dll\targz_context.dll" "%WINDIR%\System32\" >nul 2>&1
    
    :: Re-register the DLL
    echo Re-registering DLL...
    regsvr32 /u /s "%WINDIR%\System32\targz_context.dll" >nul 2>&1
    regsvr32 /s "%WINDIR%\System32\targz_context.dll"
    
    if not errorlevel 1 (
        echo.
        echo ================================================================
        echo SUCCESS: DLL rebuilt and installed with top menu positioning!
        echo ================================================================
        echo.
        echo The extract menu should now appear at the top of the context menu.
        echo Test by right-clicking on a .tar.gz or .tgz file.
        echo.
    ) else (
        echo DLL registration failed!
    )
) else (
    echo ERROR: Could not find compiled DLL at %~dp0..\dll\targz_context.dll
    echo Run build_dll.bat first.
)

pause
