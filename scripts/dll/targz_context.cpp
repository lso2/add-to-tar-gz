#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <string>
#include <vector>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")

// GUID for our context menu handler - unique GUID
static const GUID CLSID_TarGzContextMenu = 
{ 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x9a } };

class TarGzContextMenu : public IShellExtInit, public IContextMenu
{
private:
    ULONG m_cRef;
    std::vector<std::wstring> m_files;
    HBITMAP m_hMenuBitmap;

public:
    TarGzContextMenu() : m_cRef(1), m_hMenuBitmap(NULL) {}
    ~TarGzContextMenu() 
    { 
        if (m_hMenuBitmap) 
            DeleteObject(m_hMenuBitmap);
    }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
    {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellExtInit))
        {
            *ppvObj = static_cast<IShellExtInit*>(this);
        }
        else if (IsEqualIID(riid, IID_IContextMenu))
        {
            *ppvObj = static_cast<IContextMenu*>(this);
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    // IShellExtInit method
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pDataObj, HKEY hRegKey)
    {
        if (!pDataObj) return E_INVALIDARG;

        FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stm;

        if (FAILED(pDataObj->GetData(&fe, &stm)))
            return E_INVALIDARG;

        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop)
        {
            UINT uNumFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < uNumFiles; i++)
            {
                WCHAR szFile[MAX_PATH];
                if (DragQueryFileW(hDrop, i, szFile, ARRAYSIZE(szFile)))
                {
                    std::wstring file = szFile;
                    // Check if it's a .tar.gz or .tgz file
                    if (file.length() > 7 && file.substr(file.length() - 7) == L".tar.gz" ||
                        file.length() > 4 && file.substr(file.length() - 4) == L".tgz")
                    {
                        m_files.push_back(file);
                    }
                }
            }
            GlobalUnlock(stm.hGlobal);
        }
        ReleaseStgMedium(&stm);

        return m_files.empty() ? E_FAIL : S_OK;
    }

    // IContextMenu methods
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uMenuIndex, UINT uidCmdFirst, UINT uidCmdLast, UINT uFlags)
    {
        if (uFlags & CMF_DEFAULTONLY || m_files.empty())
            return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

        // Extract just the filename for display
        std::wstring filename = PathFindFileNameW(m_files[0].c_str());
        
        // Remove extension to get folder name
        std::wstring foldername = filename;
        if (foldername.length() > 7 && foldername.substr(foldername.length() - 7) == L".tar.gz")
        {
            foldername = foldername.substr(0, foldername.length() - 7);
        }
        else if (foldername.length() > 4 && foldername.substr(foldername.length() - 4) == L".tgz")
        {
            foldername = foldername.substr(0, foldername.length() - 4);
        }

        // Determine actual file extension for display
        std::wstring actualExtension;
        if (filename.length() > 7 && filename.substr(filename.length() - 7) == L".tar.gz")
        {
            actualExtension = L"tar.gz";
        }
        else if (filename.length() > 4 && filename.substr(filename.length() - 4) == L".tgz")
        {
            actualExtension = L"tgz";
        }
        else
        {
            actualExtension = L"tar.gz"; // fallback
        }

        // Create menu text: "Extract tar.gz to foldername/" or "Extract tgz to foldername/"
        std::wstring menuText = L"Extract " + actualExtension + L" to " + foldername + L"/";

        // Get icon path from registry
        WCHAR iconPath[MAX_PATH] = {0};
        HKEY hKey;
        DWORD dwSize = sizeof(iconPath);
        
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\AddToTarGz", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueExW(hKey, L"IconPath", NULL, NULL, (LPBYTE)iconPath, &dwSize);
            RegCloseKey(hKey);
        }
        
        // Fallback to default path if registry read failed
        if (wcslen(iconPath) == 0)
        {
            ExpandEnvironmentStringsW(L"%ProgramData%\\AddToTarGz\\tgz.ico", iconPath, MAX_PATH);
        }

        // Insert menu item with icon
        MENUITEMINFOW mii = { 0 };
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_STRING;
        mii.wID = uidCmdFirst;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;
        mii.dwTypeData = const_cast<LPWSTR>(menuText.c_str());

        // Try to load and set icon
        HICON hIcon = (HICON)LoadImageW(NULL, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        if (hIcon)
        {
            // Convert icon to bitmap for menu
            HDC hdcScreen = GetDC(NULL);
            if (hdcScreen)
            {
                HDC hdcMem = CreateCompatibleDC(hdcScreen);
                if (hdcMem)
                {
                    // Get icon size
                    ICONINFO iconInfo;
                    if (GetIconInfo(hIcon, &iconInfo))
                    {
                        BITMAP bmp;
                        GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp);
                        int iconWidth = bmp.bmWidth;
                        int iconHeight = bmp.bmHeight;
                        
                        if (iconWidth > 16) iconWidth = 16;
                        if (iconHeight > 16) iconHeight = 16;
                        
                        // Create bitmap
                        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, iconWidth, iconHeight);
                        if (hBitmap)
                        {
                            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                            
                            // Fill with menu background
                            RECT rect = {0, 0, iconWidth, iconHeight};
                            HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
                            FillRect(hdcMem, &rect, hBrush);
                            DeleteObject(hBrush);
                            
                            // Draw icon
                            if (DrawIconEx(hdcMem, 0, 0, hIcon, iconWidth, iconHeight, 0, NULL, DI_NORMAL))
                            {
                                m_hMenuBitmap = hBitmap;
                                mii.fMask |= MIIM_BITMAP;
                                mii.hbmpItem = hBitmap;
                            }
                            else
                            {
                                DeleteObject(hBitmap);
                            }
                            
                            SelectObject(hdcMem, hOldBitmap);
                        }
                        
                        DeleteObject(iconInfo.hbmMask);
                        DeleteObject(iconInfo.hbmColor);
                    }
                    DeleteDC(hdcMem);
                }
                ReleaseDC(NULL, hdcScreen);
            }
            DestroyIcon(hIcon);
        }

        if (InsertMenuItemW(hMenu, uMenuIndex, TRUE, &mii))
        {
            return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1);
        }

        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCmdInfo)
    {
        if (HIWORD(pCmdInfo->lpVerb) != 0 || LOWORD(pCmdInfo->lpVerb) != 0)
            return E_INVALIDARG;

        if (m_files.empty()) return E_FAIL;

        // Get script directory from registry or use default path
        WCHAR scriptPath[MAX_PATH];
        HKEY hKey;
        DWORD dwSize = sizeof(scriptPath);
        
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\AddToTarGz", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueExW(hKey, L"ScriptPath", NULL, NULL, (LPBYTE)scriptPath, &dwSize);
            RegCloseKey(hKey);
        }
        else
        {
            ExpandEnvironmentStringsW(L"%ProgramData%\\AddToTarGz\\ExtractTarGz.bat", scriptPath, MAX_PATH);
        }

        // Execute extraction
        std::wstring cmdLine = L"\"" + std::wstring(scriptPath) + L"\" \"" + m_files[0] + L"\"";
        
        STARTUPINFOW si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        if (CreateProcessW(NULL, const_cast<LPWSTR>(cmdLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return S_OK;
        }

        return E_FAIL;
    }

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
    {
        if (idCmd != 0) return E_INVALIDARG;

        if (uFlags == GCS_HELPTEXTW)
        {
            wcscpy_s(reinterpret_cast<LPWSTR>(pszName), cchMax, L"Extract archive to folder");
        }
        else if (uFlags == GCS_VERBA)
        {
            strcpy_s(pszName, cchMax, "extract_targz");
        }

        return S_OK;
    }
};

class TarGzClassFactory : public IClassFactory
{
private:
    ULONG m_cRef;

public:
    TarGzClassFactory() : m_cRef(1) {}

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
    {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
        {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() 
    { 
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    // IClassFactory methods
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;

        TarGzContextMenu *pMenu = new TarGzContextMenu();
        if (!pMenu) return E_OUTOFMEMORY;

        HRESULT hr = pMenu->QueryInterface(riid, ppvObj);
        pMenu->Release();
        return hr;
    }

    STDMETHODIMP LockServer(BOOL fLock) { return S_OK; }
};

// DLL exports
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}

extern "C" STDAPI DllCanUnloadNow()
{
    return S_OK;
}

extern "C" STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_TarGzContextMenu))
    {
        TarGzClassFactory *pFactory = new TarGzClassFactory();
        if (pFactory)
        {
            HRESULT hr = pFactory->QueryInterface(riid, ppv);
            pFactory->Release();
            return hr;
        }
        return E_OUTOFMEMORY;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" STDAPI DllRegisterServer()
{
    HKEY hKey;
    WCHAR szCLSID[MAX_PATH];
    WCHAR szSubkey[MAX_PATH];

    // Convert CLSID to string
    StringFromGUID2(CLSID_TarGzContextMenu, szCLSID, ARRAYSIZE(szCLSID));

    // Register CLSID
    swprintf_s(szSubkey, L"CLSID\\%s", szCLSID);
    RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)L"TarGz Context Menu", sizeof(L"TarGz Context Menu"));
    RegCloseKey(hKey);

    // Register InprocServer32
    swprintf_s(szSubkey, L"CLSID\\%s\\InprocServer32", szCLSID);
    RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    
    WCHAR szModule[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(L"targz_context.dll"), szModule, ARRAYSIZE(szModule));
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szModule, (wcslen(szModule) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (BYTE*)L"Apartment", sizeof(L"Apartment"));
    RegCloseKey(hKey);

    // Register for .tar.gz files specifically
    RegCreateKeyExW(HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.tar.gz\\shellex\\ContextMenuHandlers\\TarGzExtract", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szCLSID, (wcslen(szCLSID) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    // Register for .tgz files specifically  
    RegCreateKeyExW(HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.tgz\\shellex\\ContextMenuHandlers\\TarGzExtract", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szCLSID, (wcslen(szCLSID) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    return S_OK;
}

extern "C" STDAPI DllUnregisterServer()
{
    WCHAR szCLSID[MAX_PATH];
    WCHAR szSubkey[MAX_PATH];

    StringFromGUID2(CLSID_TarGzContextMenu, szCLSID, ARRAYSIZE(szCLSID));

    // Unregister context menu handlers
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.tar.gz\\shellex\\ContextMenuHandlers\\TarGzExtract");
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.tgz\\shellex\\ContextMenuHandlers\\TarGzExtract");

    // Unregister CLSID
    swprintf_s(szSubkey, L"CLSID\\%s\\InprocServer32", szCLSID);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szSubkey);
    swprintf_s(szSubkey, L"CLSID\\%s", szCLSID);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szSubkey);

    return S_OK;
}
