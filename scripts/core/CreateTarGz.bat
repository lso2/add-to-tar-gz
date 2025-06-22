@echo off
setlocal enabledelayedexpansion

:: Get input path
set "input=%~1"

:: Check if input exists
if not exist "%input%" (
    echo ERROR: Path does not exist: %input%
    timeout /t 5 /nobreak >nul
    exit /b 1
)

:: Get folder and name
for %%I in ("%input%") do (
    set "folder=%%~dpI"
    set "name=%%~nI"
    set "fullname=%%~nxI"
)

:: Remove trailing backslash from folder if present
if "%folder:~-1%"=="\" set "folder=%folder:~0,-1%"

:: Determine output filename
:: If input is a file with extension, use full name without extension
:: If input is a folder, use folder name
if exist "%input%\*" (
    :: It's a directory
    set "outputname=%name%"
) else (
    :: It's a file
    set "outputname=%name%"
)

:: Set output paths
set "tarfile=%folder%\%outputname%.tar"
set "targzfile=%folder%\%outputname%.tar.gz"

:: Check if output already exists
if exist "%targzfile%" (
    del "%targzfile%"
)

:: Find 7-Zip
set "sevenzip=%ProgramFiles%\7-Zip\7z.exe"
if not exist "%sevenzip%" set "sevenzip=%ProgramFiles(x86)%\7-Zip\7z.exe"
if not exist "%sevenzip%" (
    exit /b 1
)

:: Create TAR archive
"%sevenzip%" a -ttar "%tarfile%" "%input%" >nul 2>&1
if errorlevel 1 (
    exit /b 1
)

:: Compress TAR to TAR.GZ
"%sevenzip%" a -tgzip "%targzfile%" "%tarfile%" -mx9 >nul 2>&1
if errorlevel 1 (
    del "%tarfile%" >nul 2>&1
    exit /b 1
)

:: Clean up intermediate TAR file
del "%tarfile%" >nul 2>&1

exit /b 0
