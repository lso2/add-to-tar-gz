# Changelog

## [2.0.0] - 2026-11-14

### Added
- AddToTarGz submenu in the Explorer menu, with an option for entries in the main menu.
- Extract here, Extract to "name\" and Extract to... entries for archives.
- Settings window for the archive extension, compression level, menu entries and tray icon.
- Tray icon with a menu for the archive extension, compression level and submenu.
- Start menu shortcut to the settings window.
- Update check against the GitHub releases in the settings window, with a Download button for a newer version.
- GitHub link and Buy me a coffee button in the settings window.
- Buy me a coffee entry in the tray menu.
- Compression level choice of 1, 5 or 9.
- Optional Add entry for the other extension.
- 7-Zip 26.06 console program included in the installer.
- Add and extract jobs run without a console window.
- Progress window with a Cancel button for jobs longer than half a second.
- Menu entries show the exact name of the archive or folder that clicking them creates.
- Selecting multiple items creates one archive per item, and selecting multiple archives creates one result per archive.
- Add to "folder.tar.gz" entry for multiple selected items, creating one archive that holds all of them.
- Report dialog with 7-Zip's exit codes and messages for any job that does not finish cleanly.
- Command line options --add, --one, --extract, --ext, --level, --here, --dest, --pick-dest and --silent, with exit codes.
- Per-user Inno Setup 7 installer.
- Upgrades install while Explorer has the menu loaded, with no Explorer restart, computer restart or prompt.
- Visual Studio solution and build script for Visual Studio 2026.

### Changed
- Installs per user into %LOCALAPPDATA%\AddToTarGz, with no administrator rights.
- 7-Zip is included, with no separate install.
- The Explorer menu is registered once.
- Extraction is done by 7-Zip, the same tool as compression.
- Archives are compressed at gzip level 5 by default.
- Unfinished output has a .partial suffix.

### Fixed
- Folder names containing dots are kept whole in the archive name.
- An existing archive or folder is never overwritten.
- A .tar file beside the item is left untouched.
- An archive whose root folder has the archive's own name is extracted one level deep.
- Names with !, %, &, parentheses, Unicode letters, a leading - or @, and paths past 260 characters are supported.
- Failed jobs show an error dialog.
- The installed copy has no dependency on the source folder.
- The menu DLL is not unloaded while a menu is open.

### Removed
- INSTALL.bat, UNINSTALL.bat, install-wizard.bat, the .reg templates, the PowerShell scripts and the utility scripts.
- The WSL extraction fallback.

## [1.3.2] - 2025-06-22

### Changed
- Running the installer or the wizard re-registers the context menu DLL.
- The 7-Zip requirement is listed throughout the documentation.

### Fixed
- Extract here appears at the top of the archive menu.
- An existing installation is detected by the install wizard.

## [1.3.1]

### Changed
- Minor improvements.

## [1.3.0]

### Added
- Choice of .tar.gz or .tgz in the installer.
- Running the installer again updates an existing installation.
- Context menu text matches the chosen extension.

### Changed
- Every file is copied to %ProgramData%\AddToTarGz during installation.
- The source folder can be deleted after installation.

### Fixed
- The uninstaller works after the source folder is deleted.

## [1.2.0]

### Added
- Separate folders for core scripts, the DLL, development tools and utilities.

### Changed
- Reorganized folder structure.
- Clearer context menu text.
- Running INSTALL.bat installs silently in one click.
- The new folder structure is documented.

### Fixed
- Icons in the DLL-based context menu.

## [1.0.0]

### Added
- Add to tar.gz for files and folders.
- Extraction of tar.gz and tgz files.
- Windows Explorer context menu integration.
- Custom tgz icon.
- Removal of the intermediate .tar file after compression.
- Quick installer that deploys the icon.
- PowerShell script versions.
