@echo off
setlocal enabledelayedexpansion

set "input=%~1"

:: Get the directory and filename
for %%I in ("%input%") do (
    set "folder=%%~dpI"
    set "filename=%%~nxI"
    set "basename=%%~nI"
)

:: Remove trailing backslash from folder if present
if "%folder:~-1%"=="\" set "folder=%folder:~0,-1%"

:: Create extraction folder name - remove .tar.gz or .tgz properly
if /i "%filename:~-7%"==".tar.gz" (
    set "extractFolder=%folder%\%basename:~0,-4%"
) else if /i "%filename:~-4%"==".tgz" (
    set "extractFolder=%folder%\%basename%"
) else (
    exit /b 1
)

:: Create the extraction folder
if not exist "%extractFolder%" mkdir "%extractFolder%"

:: Use Windows built-in tar command (one step extraction)
cd /d "%extractFolder%"
tar -xzf "%input%" >nul 2>&1
if errorlevel 1 (
    :: Fallback to WSL tar if Windows tar fails
    wsl tar -xzf "%input%" >nul 2>&1
)

exit /b 0
