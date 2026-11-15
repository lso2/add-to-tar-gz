; Installer.iss: AddToTarGz per-user installer for Inno Setup 7 (https://jrsoftware.org/isinfo.php).
; Installs AddToTarGz.exe, AddToTarGzTray.exe, targz_context.dll and 7za.exe into a build folder in %LOCALAPPDATA%\AddToTarGz,
; registers the Explorer menu handler under HKCU, starts the tray icon and adds a Start menu shortcut to the
; settings window. Runs without administrator rights.
; The binaries in [Files] are read from bin\x64\Release, the output of build.ps1. The version number is read from the built AddToTarGz.exe.
; Doc: docs/notes/Installer.iss.md

#define MyAppName "AddToTarGz"
#define MyAppPublisher "PlexPixel"
#define MyAppExe "AddToTarGz.exe"
#define MyTrayExe "AddToTarGzTray.exe"
#define MyMenuDll "targz_context.dll"

#define VerMajor
#define VerMinor
#define VerRev
#define VerBuild
#define FullVersion GetVersionComponents(AddBackslash(SourcePath) + "bin\x64\Release\" + MyAppExe, VerMajor, VerMinor, VerRev, VerBuild)
#define MyAppVersion Str(VerMajor) + "." + Str(VerMinor) + "." + Str(VerRev)
; Each compiled setup installs its binaries into a folder of its own. A file another program holds, such as
; targz_context.dll loaded in Explorer, is then never written: the new build goes beside it.
#define BuildStamp GetDateTimeString('yymmddhhnnss', '', '')
#define BuildFolder "build-" + BuildStamp
; This build's class identifier for the menu handler, in the family src\menu\clsid.h accepts. A running Explorer
; with an earlier build loaded then creates the next menu from this build's DLL. Inno reads a lone "{" as the start
; of a constant, and "{{" is a literal brace, the same escape AppId uses.
#define MyClsid "{{BF8A488E-EBA4-4D70-8486-" + BuildStamp + "}"
#define MyClsidName "{BF8A488E-EBA4-4D70-8486-" + BuildStamp + "}"

[Setup]
AppId={{E2E7F486-03D0-4B2F-9FCF-8AE27988F790}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://plexpixel.com
AppSupportURL=https://github.com/lso2/add-to-tar-gz
DefaultDirName={localappdata}\{#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=lowest
OutputDir=Output
OutputBaseFilename=AddToTarGzSetup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Setup closes no program except the tray app (CloseTrayApp), and [Run] starts the new one.
CloseApplications=no
; Explorer reads the new class identifier of the menu handler at the next right-click.
ChangesAssociations=yes
UninstallDisplayIcon={app}\{#BuildFolder}\{#MyAppExe}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; onlyifdoesntexist: running the same setup again leaves its own build folder as it is, also with files in use.
; The uninstaller removes targz_context.dll in [Code], also when Explorer still has it loaded.
Source: "bin\x64\Release\{#MyAppExe}"; DestDir: "{app}\{#BuildFolder}"; Flags: onlyifdoesntexist
Source: "bin\x64\Release\{#MyTrayExe}"; DestDir: "{app}\{#BuildFolder}"; Flags: onlyifdoesntexist
Source: "bin\x64\Release\{#MyMenuDll}"; DestDir: "{app}\{#BuildFolder}"; Flags: onlyifdoesntexist uninsneveruninstall
; 7-Zip's standalone console version, the only 7-Zip the worker runs, with the license text that must ship beside it.
Source: "bin\x64\Release\7za.exe"; DestDir: "{app}\{#BuildFolder}"; Flags: onlyifdoesntexist
Source: "third_party\7-Zip\License.txt"; DestDir: "{app}\{#BuildFolder}"; DestName: "7-Zip License.txt"; Flags: onlyifdoesntexist
Source: "LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{userprograms}\AddToTarGz Settings"; Filename: "{app}\{#BuildFolder}\{#MyTrayExe}"; Parameters: "--settings"; Comment: "Archive extension, compression level, Explorer menu items and tray icon"

[Registry]
; COM class of the menu handler, registered for this user only.
Root: HKCU; Subkey: "Software\Classes\CLSID\{#MyClsid}"; ValueType: string; ValueName: ""; ValueData: "AddToTarGz context menu"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{#MyClsid}\InprocServer32"; ValueType: string; ValueName: ""; ValueData: "{app}\{#BuildFolder}\{#MyMenuDll}"
Root: HKCU; Subkey: "Software\Classes\CLSID\{#MyClsid}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
; One handler entry covers files and folders. The handler picks the items from the selection and the settings.
Root: HKCU; Subkey: "Software\Classes\AllFilesystemObjects\shellex\ContextMenuHandlers\AddToTarGz"; ValueType: string; ValueName: ""; ValueData: "{#MyClsid}"; Flags: uninsdeletekey
; Settings written by the settings window and the tray menu. A missing value means the default in src\shared\settings.h.
Root: HKCU; Subkey: "Software\AddToTarGz"; Flags: uninsdeletekey
; The same command text autostart.cpp writes when the tray icon is turned on in the settings window.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "AddToTarGz"; ValueData: """{app}\{#BuildFolder}\{#MyTrayExe}"" --tray"; Check: TrayIconEnabled

[Run]
; With TrayIcon set to 0, AddToTarGzTray.exe --tray exits with no icon or window.
Filename: "{app}\{#BuildFolder}\{#MyTrayExe}"; Parameters: "--tray"; Flags: nowait
Filename: "{app}\{#BuildFolder}\{#MyTrayExe}"; Parameters: "--settings"; Description: "Open AddToTarGz Settings"; Flags: postinstall nowait skipifsilent

[Code]
const
  WM_CLOSE = $0010;
  RunKey = 'Software\Microsoft\Windows\CurrentVersion\Run';
  // kTrayWindowClass in src\tray\tray_window.h.
  TrayWindowClass = 'AddToTarGzTrayWindow';

// The default of Settings::trayIcon in src\shared\settings.h: on unless TrayIcon is 0.
function TrayIconEnabled: Boolean;
var
  Value: Cardinal;
begin
  Result := not (RegQueryDWordValue(HKEY_CURRENT_USER, 'Software\AddToTarGz', 'TrayIcon', Value) and (Value = 0));
end;

// Earlier 2.0.0 builds registered a separate Extract command and two menu switches.
procedure RemoveOldRegistration;
begin
  RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\*\shell\AddToTarGz.Extract');
  RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\CLSID\{440A816B-AE11-44E6-9921-FAF72C41C489}');
  RegDeleteValue(HKEY_CURRENT_USER, 'Software\AddToTarGz', 'ShowAdd');
  RegDeleteValue(HKEY_CURRENT_USER, 'Software\AddToTarGz', 'ShowExtract');
end;

procedure CloseTrayApp;
var
  Waited: Integer;
begin
  if FindWindowByClassName(TrayWindowClass) = 0 then
    Exit;
  PostMessage(FindWindowByClassName(TrayWindowClass), WM_CLOSE, 0, 0);
  Waited := 0;
  while (FindWindowByClassName(TrayWindowClass) <> 0) and (Waited < 5000) do
  begin
    Sleep(100);
    Waited := Waited + 100;
  end;
end;

// A file another program has loaded stays. The tray app removes it at a later start (src\tray\old_builds.cpp).
procedure RemoveFile(const Path: String);
begin
  if not FileExists(Path) then
    Exit;
  if DeleteFile(Path) then
    Log('Removed ' + Path)
  else
    Log('In use, left for the tray app: ' + Path);
end;

// The files a build folder holds. Builds before build folders had them in {app} itself.
procedure RemoveBinaries(const Folder: String);
begin
  RemoveFile(Folder + '\{#MyAppExe}');
  RemoveFile(Folder + '\{#MyTrayExe}');
  RemoveFile(Folder + '\{#MyMenuDll}');
  RemoveFile(Folder + '\7za.exe');
  RemoveFile(Folder + '\7-Zip License.txt');
end;

// Every AddToTarGz class identifier except Keep. An earlier build's DLL loaded in Explorer keeps working without its
// key, and Explorer creates new menus from the class registered by this build.
procedure RemoveOtherClasses(const Keep: String);
var
  Names: TArrayOfString;
  I: Integer;
begin
  if not RegGetSubkeyNames(HKEY_CURRENT_USER, 'Software\Classes\CLSID', Names) then
    Exit;
  for I := 0 to GetArrayLength(Names) - 1 do
    if (CompareText(Copy(Names[I], 1, 25), '{BF8A488E-EBA4-4D70-8486-') = 0) and (CompareText(Names[I], Keep) <> 0) then
      if RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Classes\CLSID\' + Names[I]) then
        Log('Removed class ' + Names[I]);
end;

// Every build except Keep. A build folder is removed once it is empty.
procedure RemoveOtherBuilds(const Keep: String);
var
  Find: TFindRec;
  Folder: String;
begin
  RemoveBinaries(ExpandConstant('{app}'));
  if FindFirst(ExpandConstant('{app}\build-*'), Find) then
  try
    repeat
      if ((Find.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0) and (CompareText(Find.Name, Keep) <> 0) then
      begin
        Folder := ExpandConstant('{app}\') + Find.Name;
        RemoveBinaries(Folder);
        RemoveDir(Folder);
      end;
    until not FindNext(Find);
  finally
    FindClose(Find);
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  CloseTrayApp;
  Result := '';
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
    RemoveOldRegistration;
  if CurStep = ssPostInstall then
  begin
    RemoveOtherClasses('{#MyClsidName}');
    RemoveOtherBuilds('{#BuildFolder}');
  end;
end;

// A menu DLL still loaded in Explorer at uninstall cannot be deleted, and a loaded file can be moved within its
// drive. It goes to the temp folder, and the AddToTarGz folder is removed completely.
procedure RemoveOrMoveMenuDll(const Path: String);
var
  Moved: String;
begin
  if not FileExists(Path) then
    Exit;
  if DeleteFile(Path) then
  begin
    Log('Removed ' + Path);
    Exit;
  end;
  Moved := AddBackslash(GetTempDir) + 'AddToTarGz-' + GetDateTimeString('yyyymmdd-hhnnss', #0, #0) + '-{#MyMenuDll}';
  if RenameFile(Path, Moved) then
    Log('In use, moved to ' + Moved)
  else
    Log('In use, left in place: ' + Path);
end;

procedure RemoveAllBuilds;
var
  Find: TFindRec;
  Folder: String;
begin
  RemoveOrMoveMenuDll(ExpandConstant('{app}\{#MyMenuDll}'));
  RemoveBinaries(ExpandConstant('{app}'));
  if FindFirst(ExpandConstant('{app}\build-*'), Find) then
  try
    repeat
      if (Find.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0 then
      begin
        Folder := ExpandConstant('{app}\') + Find.Name;
        RemoveOrMoveMenuDll(Folder + '\{#MyMenuDll}');
        RemoveBinaries(Folder);
        RemoveDir(Folder);
      end;
    until not FindNext(Find);
  finally
    FindClose(Find);
  end;
  RemoveDir(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    CloseTrayApp;
    RegDeleteValue(HKEY_CURRENT_USER, RunKey, 'AddToTarGz');
  end;
  if CurUninstallStep = usPostUninstall then
  begin
    RemoveOtherClasses('');
    RemoveAllBuilds;
  end;
end;
