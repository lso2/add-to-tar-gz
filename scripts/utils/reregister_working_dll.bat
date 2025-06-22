@echo off
echo Re-registering the working DLL...

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Must run as Administrator
    pause
    exit /b 1
)

:: Make sure the DLL exists in system32
if not exist "%WINDIR%\System32\targz_context.dll" (
    echo Copying DLL to system32...
    copy /Y "..\dll\targz_context.dll" "%WINDIR%\System32\" >nul 2>&1
)

:: Re-register the DLL
echo Registering DLL...
regsvr32 /s "%WINDIR%\System32\targz_context.dll"

:: Make sure registry entries exist
reg add "HKLM\SOFTWARE\AddToTarGz" /v "ScriptPath" /t REG_SZ /d "%~dp0..\core\ExtractTarGz.bat" /f >nul

echo Done! Test right-clicking a .tar.gz file.
if "%1" neq "silent" pause
