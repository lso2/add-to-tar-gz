@echo off
echo Updating TarGz Context Menu DLL...
echo.

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: This script must be run as Administrator.
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Unregister old DLL
echo Unregistering old DLL...
regsvr32 /u /s "%WINDIR%\System32\targz_context.dll" >nul 2>&1

:: Build new DLL
echo Building new DLL...
call ..\dev\build_dll.bat
if errorlevel 1 (
    echo Failed to build DLL!
    pause
    exit /b 1
)

:: Copy new DLL
echo Installing new DLL...
copy /Y "..\dll\targz_context.dll" "%WINDIR%\System32\" >nul 2>&1
if errorlevel 1 (
    echo Failed to copy DLL!
    pause
    exit /b 1
)

:: Register new DLL
echo Registering new DLL...
regsvr32 /s "%WINDIR%\System32\targz_context.dll"
if errorlevel 1 (
    echo Failed to register DLL!
    pause
    exit /b 1
)

echo.
echo SUCCESS: DLL updated successfully!
echo Try right-clicking a .tar.gz file to test.
echo.
pause
