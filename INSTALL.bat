@echo off
setlocal enabledelayedexpansion
:: =========================================
:: Add to tar.gz - Silent Installer v1.3.2
:: =========================================

echo =========================================
echo Add to tar.gz - Installer v1.3.2
echo =========================================
echo.
echo Installing Windows context menu integration...
echo.

:: Check if running as Administrator
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: This installer must be run as Administrator.
    echo Please right-click on INSTALL.bat and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo Running with Administrator privileges... OK
echo.

:: Get current directory
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

:: Set installation destination in ProgramData
set "INSTALL_DIR=%ProgramData%\AddToTarGz"
set "ICON_PATH=%INSTALL_DIR%\tgz.ico"
set "EXTRACT_SCRIPT=%INSTALL_DIR%\ExtractTarGz.bat"
set "CREATE_SCRIPT=%INSTALL_DIR%\CreateTarGz.bat"
set "UNINSTALL_SCRIPT=%INSTALL_DIR%\uninstall.bat"

:: Default to .tar.gz extension for silent install
set "EXTENSION=tar.gz"
set "MENU_TEXT=Add to tar.gz"
set "MENU_KEY=AddToTarGz"

:: Check for existing installation and remove silently
if exist "%INSTALL_DIR%" (
    call "%UNINSTALL_SCRIPT%" silent >nul 2>&1
    timeout /t 2 /nobreak >nul
)

:: Create installation directory and copy files
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%" 2>nul
copy /Y "%SCRIPT_DIR%\icon\tgz.ico" "%ICON_PATH%" >nul 2>&1
copy /Y "%SCRIPT_DIR%\scripts\core\ExtractTarGz.bat" "%EXTRACT_SCRIPT%" >nul 2>&1

:: Create compression script for tar.gz
call :CREATE_COMPRESSION_SCRIPT "%CREATE_SCRIPT%" "%EXTENSION%"

:: Create uninstall script
call :CREATE_UNINSTALL_SCRIPT "%UNINSTALL_SCRIPT%"

:: Check if pre-compiled DLL exists and install it
set "DLL_INSTALLED=0"
if exist "%SCRIPT_DIR%\scripts\dll\targz_context.dll" (
    echo Installing DLL...
    copy /Y "%SCRIPT_DIR%\scripts\dll\targz_context.dll" "%WINDIR%\System32\" >nul 2>&1
    if not errorlevel 1 (
        echo Registering DLL COM object...
        regsvr32 /s "%WINDIR%\System32\targz_context.dll" >nul 2>&1
        if not errorlevel 1 (
            echo Manually registering shell extension entries...
            
            :: Get the CLSID from the DLL and register shell extension manually
            set "CLSID={A1B2C3D4-E5F6-7890-ABCD-EF123456789A}"
            
            :: Register for .tar.gz files
            reg add "HKCR\SystemFileAssociations\.tar.gz\shellex\ContextMenuHandlers\TarGzExtract" /ve /t REG_SZ /d "{A1B2C3D4-E5F6-7890-ABCD-EF123456789A}" /f >nul 2>&1
            
            :: Register for .tgz files
            reg add "HKCR\SystemFileAssociations\.tgz\shellex\ContextMenuHandlers\TarGzExtract" /ve /t REG_SZ /d "{A1B2C3D4-E5F6-7890-ABCD-EF123456789A}" /f >nul 2>&1
            
            :: Store configuration in registry
            reg add "HKLM\SOFTWARE\AddToTarGz" /v "ScriptPath" /t REG_SZ /d "%EXTRACT_SCRIPT%" /f >nul 2>&1
            reg add "HKLM\SOFTWARE\AddToTarGz" /v "IconPath" /t REG_SZ /d "%ICON_PATH%" /f >nul 2>&1
            reg add "HKLM\SOFTWARE\AddToTarGz" /v "Extension" /t REG_SZ /d "%EXTENSION%" /f >nul 2>&1
            
            echo DLL extraction menus installed at top of context menu
            set "DLL_INSTALLED=1"
        ) else (
            echo DLL registration failed, running reregister script...
            call "%SCRIPT_DIR%\scripts\utils\reregister_working_dll.bat" silent
            if not errorlevel 1 (
                set "DLL_INSTALLED=1"
                echo DLL successfully re-registered
            ) else (
                echo DLL re-registration also failed
            )
        )
    )
)

:: Clean up any existing entries first
reg delete "HKCR\*\shell\AddToTarGz" /f >nul 2>&1
reg delete "HKCR\*\shell\AddToTgz" /f >nul 2>&1
reg delete "HKCR\Directory\shell\AddToTarGz" /f >nul 2>&1
reg delete "HKCR\Directory\shell\AddToTgz" /f >nul 2>&1
reg delete "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /f >nul 2>&1
reg delete "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /f >nul 2>&1

:: Install compression menus
reg add "HKCR\*\shell\%MENU_KEY%" /ve /t REG_SZ /d "%MENU_TEXT%" /f >nul 2>&1
reg add "HKCR\*\shell\%MENU_KEY%" /v "Icon" /t REG_SZ /d "%ICON_PATH%" /f >nul 2>&1
reg add "HKCR\*\shell\%MENU_KEY%" /v "AppliesTo" /t REG_SZ /d "NOT System.FileName:\"*.tar.gz\" AND NOT System.FileName:\"*.tgz\"" /f >nul 2>&1
reg add "HKCR\*\shell\%MENU_KEY%\command" /ve /t REG_SZ /d "\"%CREATE_SCRIPT%\" \"%%1\"" /f >nul 2>&1

reg add "HKCR\Directory\shell\%MENU_KEY%" /ve /t REG_SZ /d "%MENU_TEXT%" /f >nul 2>&1
reg add "HKCR\Directory\shell\%MENU_KEY%" /v "Icon" /t REG_SZ /d "%ICON_PATH%" /f >nul 2>&1
reg add "HKCR\Directory\shell\%MENU_KEY%\command" /ve /t REG_SZ /d "\"%CREATE_SCRIPT%\" \"%%1\"" /f >nul 2>&1

:: Always install static extraction menus with Position=Top for better positioning
echo Installing static registry-based extraction menus with top positioning...

:: Register extraction for .tar.gz files at top position
reg add "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /ve /t REG_SZ /d "Extract here" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /v "Icon" /t REG_SZ /d "%ICON_PATH%" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /v "Position" /t REG_SZ /d "Top" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz\command" /ve /t REG_SZ /d "\"%EXTRACT_SCRIPT%\" \"%%1\"" /f >nul 2>&1

:: Register extraction for .tgz files at top position
reg add "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /ve /t REG_SZ /d "Extract here" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /v "Icon" /t REG_SZ /d "%ICON_PATH%" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /v "Position" /t REG_SZ /d "Top" /f >nul 2>&1
reg add "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz\command" /ve /t REG_SZ /d "\"%EXTRACT_SCRIPT%\" \"%%1\"" /f >nul 2>&1

echo Static extraction menus installed at top of context menu

:: Always run the reregister script to ensure proper DLL setup
echo.
echo Running reregister script to ensure DLL is properly configured...
call "%SCRIPT_DIR%\scripts\utils\reregister_working_dll.bat" silent
if not errorlevel 1 (
    echo DLL configuration completed successfully
) else (
    echo Warning: DLL configuration may have issues
)

echo.
echo =========================================
echo Installation completed successfully.
echo =========================================
echo Installed to: %INSTALL_DIR%
echo The source folder can now be safely deleted.
echo To uninstall, run: %UNINSTALL_SCRIPT%
echo.
echo Press any key to exit...
pause >nul
exit /b 0

:: Function to create compression script
:CREATE_COMPRESSION_SCRIPT
set "output_file=%~1"
set "extension=%~2"

(
echo @echo off
echo setlocal enabledelayedexpansion
echo.
echo :: Get input path
echo set "input=%%~1"
echo.
echo :: Check if input exists
echo if not exist "%%input%%" ^(
echo     echo ERROR: Path does not exist: %%input%%
echo     timeout /t 5 /nobreak ^>nul
echo     exit /b 1
echo ^)
echo.
echo :: Get folder and name
echo for %%%%I in ^("%%input%%"^) do ^(
echo     set "folder=%%%%~dpI"
echo     set "name=%%%%~nI"
echo     set "fullname=%%%%~nxI"
echo ^)
echo.
echo :: Remove trailing backslash from folder if present
echo if "%%folder:~-1%%"=="\" set "folder=%%folder:~0,-1%%"
echo.
echo :: Determine output filename
echo if exist "%%input%%\*" ^(
echo     :: It's a directory
echo     set "outputname=%%name%%"
echo ^) else ^(
echo     :: It's a file
echo     set "outputname=%%name%%"
echo ^)
echo.
if "%extension%"=="tar.gz" (
echo :: Set output paths for tar.gz
echo set "tarfile=%%folder%%\%%outputname%%.tar"
echo set "targzfile=%%folder%%\%%outputname%%.tar.gz"
echo.
echo :: Check if output already exists
echo if exist "%%targzfile%%" del "%%targzfile%%"
) else (
echo :: Set output paths for tgz
echo set "tarfile=%%folder%%\%%outputname%%.tar"
echo set "tgzfile=%%folder%%\%%outputname%%.tgz"
echo.
echo :: Check if output already exists
echo if exist "%%tgzfile%%" del "%%tgzfile%%"
)
echo.
echo :: Find 7-Zip
echo set "sevenzip=%%ProgramFiles%%\7-Zip\7z.exe"
echo if not exist "%%sevenzip%%" set "sevenzip=%%ProgramFiles^(x86^)%%\7-Zip\7z.exe"
echo if not exist "%%sevenzip%%" exit /b 1
echo.
echo :: Create TAR archive
echo "%%sevenzip%%" a -ttar "%%tarfile%%" "%%input%%" ^>nul 2^>^&1
echo if errorlevel 1 exit /b 1
echo.
if "%extension%"=="tar.gz" (
echo :: Compress TAR to TAR.GZ
echo "%%sevenzip%%" a -tgzip "%%targzfile%%" "%%tarfile%%" -mx9 ^>nul 2^>^&1
echo if errorlevel 1 ^(
echo     del "%%tarfile%%" ^>nul 2^>^&1
echo     exit /b 1
echo ^)
) else (
echo :: Compress TAR to TGZ
echo "%%sevenzip%%" a -tgzip "%%tgzfile%%" "%%tarfile%%" -mx9 ^>nul 2^>^&1
echo if errorlevel 1 ^(
echo     del "%%tarfile%%" ^>nul 2^>^&1
echo     exit /b 1
echo ^)
)
echo.
echo :: Clean up intermediate TAR file
echo del "%%tarfile%%" ^>nul 2^>^&1
echo.
echo exit /b 0
) > "%output_file%"

goto :eof

:: Function to create uninstall script
:CREATE_UNINSTALL_SCRIPT
set "output_file=%~1"

(
echo @echo off
echo echo =========================================
echo echo Add to tar.gz - Uninstaller v1.3.2
echo echo =========================================
echo echo.
echo.
echo :: Check if running silently
echo set "SILENT=0"
echo if "%%1"=="silent" set "SILENT=1"
echo.
echo :: Check if running as Administrator
echo net session ^>nul 2^>^&1
echo if errorlevel 1 ^(
echo     if "%%SILENT%%"=="0" ^(
echo         echo ERROR: This uninstaller must be run as Administrator.
echo         echo Please right-click and select "Run as administrator"
echo         pause
echo     ^)
echo     exit /b 1
echo ^)
echo.
echo if "%%SILENT%%"=="0" echo Running with Administrator privileges... OK
echo.
echo :: Unregister DLL if it exists
echo if exist "%%WINDIR%%\System32\targz_context.dll" ^(
echo     if "%%SILENT%%"=="0" echo Unregistering DLL...
echo     regsvr32 /u /s "%%WINDIR%%\System32\targz_context.dll" ^>nul 2^>^&1
echo     del "%%WINDIR%%\System32\targz_context.dll" ^>nul 2^>^&1
echo ^)
echo.
echo :: Remove DLL registry entries manually
echo reg delete "HKCR\SystemFileAssociations\.tar.gz\shellex\ContextMenuHandlers\TarGzExtract" /f ^>nul 2^>^&1
echo reg delete "HKCR\SystemFileAssociations\.tgz\shellex\ContextMenuHandlers\TarGzExtract" /f ^>nul 2^>^&1
echo.
echo :: Remove registry entries
echo if "%%SILENT%%"=="0" echo Removing registry entries...
echo reg delete "HKCR\*\shell\AddToTarGz" /f ^>nul 2^>^&1
echo reg delete "HKCR\*\shell\AddToTgz" /f ^>nul 2^>^&1
echo reg delete "HKCR\Directory\shell\AddToTarGz" /f ^>nul 2^>^&1
echo reg delete "HKCR\Directory\shell\AddToTgz" /f ^>nul 2^>^&1
echo reg delete "HKCR\SystemFileAssociations\.tar.gz\shell\ExtractTarGz" /f ^>nul 2^>^&1
echo reg delete "HKCR\SystemFileAssociations\.tgz\shell\ExtractTarGz" /f ^>nul 2^>^&1
echo reg delete "HKLM\SOFTWARE\AddToTarGz" /f ^>nul 2^>^&1
echo.
echo :: Remove installation directory ^(this script will delete itself^)
echo if "%%SILENT%%"=="0" ^(
echo     echo.
echo     echo Removing installation directory...
echo     echo %%ProgramData%%\AddToTarGz
echo     echo.
echo     timeout /t 3 /nobreak ^>nul
echo ^)
echo.
echo :: Self-delete and cleanup
echo start /b "" cmd /c del /q "%%ProgramData%%\AddToTarGz\*.*" ^&^& rmdir "%%ProgramData%%\AddToTarGz" ^&^& if "%%SILENT%%"=="0" echo Uninstall complete!
echo.
echo exit /b 0
) > "%output_file%"

goto :eof
