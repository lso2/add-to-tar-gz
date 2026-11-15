# build.ps1: builds AddToTarGz.exe, AddToTarGzTray.exe and targz_context.dll (Release, x64) with the newest Visual
# Studio that vswhere finds. Output, with the bundled 7za.exe, is written to bin\x64\Release, the folder Installer.iss reads.
# Globals: none.
# Doc: docs/notes/build.ps1.md
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Definition
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "vswhere.exe was not found at $vswhere. Visual Studio 2017 and later install it there."
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' |
    Select-Object -First 1
if (-not $msbuild) {
    throw 'vswhere found no Visual Studio installation with MSBuild.'
}
& $msbuild (Join-Path $root 'AddToTarGz.slnx') /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
exit $LASTEXITCODE
