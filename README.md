# Windows Add to tar.gz Context Menu

![Windows](https://img.shields.io/badge/Windows-10%2B-blue.svg)
![Version](https://img.shields.io/badge/Version-1.3.2-green.svg)
![License](https://img.shields.io/badge/License-GPLv3-orange.svg)

A simple Windows context menu extension that adds "Add to tar.gz" functionality for files and folders, plus extraction support for tar.gz and tgz files.

## 🚀 Quick Start

1. **Download/Clone** this repository
2. **Right-click** `INSTALL.bat` → **"Run as administrator"**
3. **Right-click any non-tar file/folder** → **"Add to tar.gz"** ✓
4. **Right-click any .tar.gz/.tgz file** → **"Extract to folder/"** ✓

## ✨ Features

- **One-Click Compression** - Right-click any file or folder to create a tar.gz archive instantly
- **Extract Support** - Right-click tar.gz or tgz files to extract them to the current directory
- **Native Integration** - Seamlessly integrates with Windows Explorer context menu
- **Custom Icon** - Professional tgz icon for easy visual identification
- **Automatic Cleanup** - Removes intermediate .tar files after creating .tar.gz
- **PowerShell Alternative** - Includes PowerShell scripts for advanced users

## 📋 Requirements

- Windows 10 or higher

## 🔧 Installation

### Quick Installation (Recommended)
1. **Clone or Download** this repository
2. **Right-click on `INSTALL.bat`** and select "Run as administrator"
3. **Installation runs silently** - installs with .tar.gz extension by default
4. **Done!** Source folder can now be safely deleted

### Interactive Installation (Advanced)
1. **Navigate to `scripts/`** folder
2. **Right-click on `install-wizard.bat`** and select "Run as administrator"
3. **Choose extension**: .tar.gz (Linux-compatible) or .tgz (shorter)
4. **Choose installation type**:
   - Full installation (compression + extraction)
   - Compression only
   - Extraction only
5. **Follow prompts** for customized installation
6. **Source folder can be deleted** after installation

**💾 Portable Installation:** All required files are copied to `%ProgramData%\AddToTarGz\` during installation. The source folder can be safely deleted after installation. To uninstall later, run `%ProgramData%\AddToTarGz\uninstall.bat` as Administrator.

### Manual Installation
1. **Edit Registry File**:
   - Navigate to the `registry` folder
   - Open desired `.reg` file in a text editor
   - Replace `C:\Path\To` with your actual installation path
   - Save the file
2. **Install**:
   - Double-click the edited `.reg` file
   - Confirm the registry modification

## 📁 File Structure

```
add-to-tar-gz/
├── INSTALL.bat                      # Silent installer (one-click)
├── UNINSTALL.bat                    # Quick uninstaller
├── README.md                        # This file
├── icon/                            # Custom icon
│   └── tgz.ico                          # Context menu icon
├── registry/                        # Registry modification files
│   ├── install.reg                      # Full installation
│   ├── install-compress.reg             # Compression only
│   ├── install-extract.reg              # Extraction only
│   └── uninstall.reg                   # Uninstaller
├── powershell/                      # PowerShell script versions
│   ├── CreateTarGz.ps1                  # PS compression script
│   ├── ExtractTarGz.ps1                 # PS extraction script
│   └── README.md                        # PowerShell documentation
└── scripts/                         # Core functionality
    ├── install-wizard.bat               # Interactive installer
    ├── core/                            # Main scripts
    │   ├── CreateTarGz.bat                  # Compression script
    │   ├── ExtractTarGz.bat                 # Extraction script
    │   ├── DynamicExtract.ps1               # PowerShell dynamic extract (with UI)
    │   └── ExtractTarGzWrapper.ps1          # PowerShell wrapper
    ├── dll/                             # Shell extension (advanced)
    │   ├── targz_context.cpp                # C++ source code
    │   ├── targz_context.def                # DLL definition
    │   └── targz_context.dll                # Compiled DLL
    ├── dev/                             # Development tools
    │   ├── build_dll.bat                    # DLL compiler
    │   └── add_icon_to_dll.bat              # Icon embedding
    └── utils/                           # Utility scripts
        ├── fix_duplicate.bat                # Duplicate menu fixer
        ├── reregister_working_dll.bat       # DLL re-registration
        └── update_dll.bat                   # DLL updater
```

## 🎯 Usage

### Compress Files/Folders
1. Right-click any file or folder
2. Select "Add to tar.gz"
3. Archive will be created in the same directory

### Extract Archives
1. Right-click any .tar.gz or .tgz file
2. Select "Extract here"
3. Contents will be extracted to the current directory

## ⚙️ Configuration

### Change Script Location
Edit the registry files and update the path:
```reg
@="\"C:\\Path\\To\\Your\\CreateTarGz.bat\" \"%1\""
```

### Add .tgz Support
The extraction feature automatically works with both .tar.gz and .tgz files.

### PowerShell Version
For advanced users, PowerShell scripts are available in the `powershell` folder with the same functionality.

## 🔌 Uninstallation

### Quick Uninstallation
1. **Right-click on `UNINSTALL.bat`** and select "Run as administrator"
2. Confirm removal
3. Delete the script files (optional)

### Manual Uninstallation
1. Run `registry\uninstall.reg` to remove context menu entries
2. Delete the script files

## 🐛 Troubleshooting

### "Error accessing the registry" during installation
- **Solution:** Right-click `INSTALL.bat` and select "Run as administrator"
- Registry modifications require Administrator privileges
- The installer will fail without proper permissions

### Icon Not Showing
- Run installer as Administrator
- Icon is copied to `%ProgramData%\AddToTarGz\`
- Try restarting Windows Explorer

### Context Menu Not Appearing
- Ensure registry was properly modified (run installer as Administrator)
- Try restarting Windows Explorer or rebooting

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Test your changes thoroughly
4. Submit a pull request

## 🔄 Changelog

### v1.3.2 - Current Release
- 🔧 **Enhanced**: Automatic DLL re-registration built into installer
- 🎯 **Fixed**: Extract context menu moved to top section for better visibility
- 🚀 **Improved**: One-click installation handles DLL registration automatically

### v1.3.1 - Previous Release
- 🐛 **Fixed**: Minor improvements

### v1.3.0 - Previous Release
- 🎯 **Major**: Choose between .tar.gz and .tgz extensions in installer
- 📁 **Enhanced**: All files now installed to %ProgramData%\AddToTarGz for standalone operation
- 🔄 **Improved**: Auto-update capability - installer detects and updates existing installations
- 🗑️ **Fixed**: Uninstaller now works even if source folder is deleted
- 💾 **Portable**: Source folder can be safely deleted after installation
- 🔧 **Smart**: DLL shows dynamic text based on selected extension
- 📝 **Better**: Context menu text adapts to chosen extension (.tar.gz or .tgz)

### v1.2.0 - Previous Release
- 🗂️ Major: Reorganized folder structure for better maintainability
- 🎯 Fixed: Icon display issues in DLL-based context menus
- 📝 Enhanced: Context menu text clarity improvements
- ⚡ Improved: Silent INSTALL.bat for one-click deployment
- 🛠️ Added: Proper separation of core scripts, DLL, dev tools, and utilities
- 📖 Updated: Documentation with new file structure

### v1.0.0 - Initial Release
- ✨ Add to tar.gz for files and folders
- 📦 Extract tar.gz and tgz files
- 🎯 Windows Explorer context menu integration
- 🎨 Custom icon for visual identification
- 🧹 Automatic cleanup of temporary files
- 🚀 Quick installer with automatic icon deployment
- 📝 PowerShell script alternatives

---

**⭐ If this tool saved you time, please star the repository!**

*Made with ❤️ for the Windows power user community*
