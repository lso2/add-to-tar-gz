@echo off
echo Building TarGz Context Menu DLL...

:: Try different Visual Studio paths
set "VS_FOUND=0"

:: Check if cl.exe is already in PATH
where cl.exe >nul 2>&1
if not errorlevel 1 (
    set "VS_FOUND=1"
    goto BUILD
)

:: Try VS 2022 Community
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "VS_FOUND=1"
    goto BUILD
)

:: Try VS 2022 Professional
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "VS_FOUND=1"
    goto BUILD
)

:: Try VS 2019 Community
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "VS_FOUND=1"
    goto BUILD
)

:: Try Build Tools for Visual Studio
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    set "VS_FOUND=1"
    goto BUILD
)

:: Try older Visual Studio versions
if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64 >nul 2>&1
    set "VS_FOUND=1"
    goto BUILD
)

if "%VS_FOUND%"=="0" (
    echo.
    echo ERROR: No Visual Studio installation found.
    echo.
    echo Please install one of:
    echo - Visual Studio 2022 Community (free)
    echo - Visual Studio 2019 Community (free)
    echo - Build Tools for Visual Studio (free)
    echo.
    echo Download from: https://visualstudio.microsoft.com/downloads/
    echo.
    exit /b 1
)

:BUILD
echo Compiling shell extension DLL...

:: Move to DLL directory
cd /d "%~dp0"
cd ..\dll

:: Clean previous build
if exist targz_context.dll del targz_context.dll
if exist targz_context.obj del targz_context.obj
if exist targz_context.lib del targz_context.lib
if exist targz_context.exp del targz_context.exp

:: Compile the DLL with proper flags
cl.exe /nologo /LD /EHsc /O2 /MT /D"WIN32" /D"_WINDOWS" /D"_USRDLL" /D"UNICODE" /D"_UNICODE" targz_context.cpp /link /DEF:targz_context.def /SUBSYSTEM:WINDOWS ole32.lib oleaut32.lib shlwapi.lib shell32.lib user32.lib advapi32.lib gdi32.lib /OUT:targz_context.dll

if errorlevel 1 (
    echo.
    echo ERROR: Compilation failed!
    echo Check the error messages above.
    exit /b 1
)

:: Clean up intermediate files
if exist targz_context.obj del targz_context.obj
if exist targz_context.lib del targz_context.lib
if exist targz_context.exp del targz_context.exp

echo.
echo SUCCESS: targz_context.dll compiled successfully!
echo.
echo The DLL is ready for distribution.
echo Next: Run INSTALL.bat as Administrator to install it.
echo.
exit /b 0
