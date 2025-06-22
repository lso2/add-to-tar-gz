@echo off

:: Check if installation exists
if not exist "%ProgramData%\AddToTarGz\uninstall.bat" (
    echo =========================================
    echo Add to tar.gz - Uninstaller v1.3.2
    echo =========================================
    echo.
    echo No installation found in %ProgramData%\AddToTarGz
    echo.
    echo Cleaning up any remaining registry entries...
    
    :: Check if running as Administrator for registry cleanup
    net session >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Administrator privileges required for registry cleanup.
        echo Please right-click and select "Run as administrator"
        pause
        exit /b 1
    )
    
    :: Manual cleanup
    reg delete "HKCR\*\shell\AddToTarGz" /f >nul 2>&1
    reg delete "HKCR\*\shell\AddToTgz" /f >nul 2>&1
    reg delete "HKCR\Directory\shell\AddToTarGz" /f >nul 2>&1
    reg delete "HKCR\Directory\shell\AddToTgz" /f >nul 2>&1
    reg delete "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /f >nul 2>&1
    reg delete "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /f >nul 2>&1
    reg delete "HKLM\SOFTWARE\AddToTarGz" /f >nul 2>&1
    
    if exist "%WINDIR%\System32\targz_context.dll" (
        echo Unregistering DLL...
        regsvr32 /u /s "%WINDIR%\System32\targz_context.dll" >nul 2>&1
        del "%WINDIR%\System32\targz_context.dll" >nul 2>&1
    )
    
    :: Remove DLL registry entries manually
    reg delete "HKCR\SystemFileAssociations\.tar.gz\shellex\ContextMenuHandlers\TarGzExtract" /f >nul 2>&1
    reg delete "HKCR\SystemFileAssociations\.tgz\shellex\ContextMenuHandlers\TarGzExtract" /f >nul 2>&1
    
    echo Registry cleanup complete.
    pause
    exit /b 0
)

:: Use the proper uninstaller
call "%ProgramData%\AddToTarGz\uninstall.bat"
