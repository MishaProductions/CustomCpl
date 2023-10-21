#include "pch.h"
#include "global.h"

#define CONTROLPANEL_NAMESPACE_GUID L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\%s"
#define SHELL_EXT_APPROVED        L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\%s"

void RefreshFolderViews(UINT csidl)
{
	PIDLIST_ABSOLUTE pidl;
	if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl)))
	{
		SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, 0);
		CoTaskMemFree(pidl);
	}
}

// Structure to hold data for individual keys to register.
typedef struct
{
	HKEY  hRootKey;
	PCWSTR pszSubKey;
	PCWSTR pszClassID;
	PCWSTR pszValueName;
	BYTE* pszData;
	DWORD dwType;
	DWORD dwSize;
} REGSTRUCT;

//
//  1. The classID must be created under HKCR\CLSID.
//     a. DefaultIcon must be set to <Path and Module>,0.
//     b. InprocServer32 set to path and module.
//        i. Threading model specified as Apartment.
//     c. ShellFolder attributes must be set.
//  2. If the extension in non-rooted, its GUID is entered at the desired folder.
//  3. It must be registered as approved for Windows NT or XP.
//
STDAPI DllRegisterServer()
{
	WCHAR szFolderViewImplClassID[64], szElementClassID[64], szSubKey[MAX_PATH], szData[MAX_PATH];
	if (FAILED(StringFromGUID2(CLSID_FolderViewImpl, szFolderViewImplClassID, ARRAYSIZE(szFolderViewImplClassID))))
	{
		return GetLastError();
	}
	if (FAILED(StringFromGUID2(CLSID_FolderViewImplElement, szElementClassID, ARRAYSIZE(szElementClassID))))
	{
		return GetLastError();
	}

	// Get the path and module name.
	WCHAR szModulePathAndName[MAX_PATH];
	GetModuleFileNameW(g_hInst, szModulePathAndName, ARRAYSIZE(szModulePathAndName));

	// This will setup and register the basic ClassIDs. 
	DWORD dwData = 0xa0000000;// SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANDELETE;
	DWORD one = 1;
	DWORD dwResourceId = IDR_PAGEDEF;

	REGSTRUCT rgRegEntries[] =
	{
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szFolderViewImplClassID, NULL,                       (LPBYTE)L"@%s,-101",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szFolderViewImplClassID, L"InfoTip",                 (LPBYTE)L"Customize Settings...",   REG_SZ,0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szFolderViewImplClassID, L"System.ApplicationName",  (LPBYTE)L"CustomCpl.ControlPanelMain",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szFolderViewImplClassID, L"System.ControlPanel.Category",(LPBYTE)L"1,5",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szFolderViewImplClassID, L"System.Software.TasksFileUrl",(LPBYTE)L"%s,-110",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\InprocServer32", szFolderViewImplClassID, NULL,                       (LPBYTE)L"C:\\Windows\\System32\\Shdocvw.dll",          REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\InprocServer32", szFolderViewImplClassID, L"ThreadingModel",          (LPBYTE)L"Apartment",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\DefaultIcon",    szFolderViewImplClassID, NULL,                       (LPBYTE)L"%s,0",        REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\ShellFolder",    szFolderViewImplClassID, L"Attributes",              (LPBYTE)&dwData,        REG_DWORD, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\Instance", szFolderViewImplClassID,    L"CLSID",          (LPBYTE)L"{328B0346-7EAF-4BBE-A479-7CB88A095F5B}",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\Instance\\InitPropertyBag", szFolderViewImplClassID,    L"ResourceDLL",          (LPBYTE)L"%s",   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\Instance\\InitPropertyBag", szFolderViewImplClassID,    L"ResourceID",          (LPBYTE)&dwResourceId,   REG_DWORD, 0,

		//element provider
		HKEY_CLASSES_ROOT,  L"CLSID\\%s",                 szElementClassID, NULL,                       (LPBYTE)g_szExtTitle,   REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\InprocServer32", szElementClassID, NULL,                       (LPBYTE)L"%s",          REG_SZ, 0,
		HKEY_CLASSES_ROOT,  L"CLSID\\%s\\InprocServer32", szElementClassID, L"ThreadingModel",          (LPBYTE)L"Apartment",   REG_SZ, 0,
	};

	HKEY hKey = NULL;
	HRESULT hr = S_OK;

	for (int i = 0; SUCCEEDED(hr) && (i < ARRAYSIZE(rgRegEntries)); i++)
	{
		// Create the sub key string.
		hr = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), rgRegEntries[i].pszSubKey, rgRegEntries[i].pszClassID);
		if (SUCCEEDED(hr))
		{
			LONG lResult = RegCreateKeyExW(rgRegEntries[i].hRootKey, szSubKey, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
			if (S_OK == lResult)
			{
				// If this is a string entry, create the string.
				if (REG_SZ == rgRegEntries[i].dwType)
				{
					hr = StringCchPrintfW(szData, ARRAYSIZE(szData), (LPWSTR)rgRegEntries[i].pszData, szModulePathAndName);
					if (SUCCEEDED(hr))
					{
						RegSetValueExW(hKey,
							rgRegEntries[i].pszValueName,
							0,
							rgRegEntries[i].dwType,
							(LPBYTE)szData,
							(lstrlenW(szData) + 1) * sizeof(WCHAR));
					}
				}
				else if (REG_EXPAND_SZ == rgRegEntries[i].dwType)
				{
					RegSetValueExW(hKey,
						rgRegEntries[i].pszValueName,
						0,
						rgRegEntries[i].dwType,
						(LPBYTE)rgRegEntries[i].pszData,
						(lstrlenW((LPWSTR)rgRegEntries[i].pszData) + 1) * sizeof(WCHAR));
				}
				else if (REG_DWORD == rgRegEntries[i].dwType)
				{
					RegSetValueExW(hKey,
						rgRegEntries[i].pszValueName,
						0, rgRegEntries[i].dwType,
						rgRegEntries[i].pszData,
						sizeof(DWORD));
				}
				else if (REG_BINARY == rgRegEntries[i].dwType)
				{
					RegSetValueExW(hKey,
						rgRegEntries[i].pszValueName,
						0, rgRegEntries[i].dwType,
						rgRegEntries[i].pszData,
						rgRegEntries[i].dwSize);
				}

				RegCloseKey(hKey);
			}
			else
			{
				hr = SELFREG_E_CLASS;
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = SELFREG_E_CLASS;

		// Now we are ready to register the namespace extension.
		// This will put our extension in My Computer.
		if (SUCCEEDED(StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), CONTROLPANEL_NAMESPACE_GUID, szFolderViewImplClassID)))
		{
			LONG lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
			if (S_OK == lResult)
			{
				// Copy our name into the string.
				hr = StringCchCopyW(szData, ARRAYSIZE(szData), g_szExtTitle);
				if (SUCCEEDED(hr))
				{
					// Set the name of our extension.
					lResult = RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)szData, (lstrlenW(szData) + 1) * sizeof(szData[0]));
					RegCloseKey(hKey);

					PCWSTR rgszApprovedClassIDs[] = { szFolderViewImplClassID };
					for (int i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(rgszApprovedClassIDs); i++)
					{
						hr = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), SHELL_EXT_APPROVED, szFolderViewImplClassID);
						if (SUCCEEDED(hr))
						{
							lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
								KEY_WRITE, NULL, &hKey, NULL);
							if (S_OK == lResult)
							{
								// Create the value string.
								hr = StringCchCopyW(szData, ARRAYSIZE(szData), g_szExtTitle);
								if (SUCCEEDED(hr))
								{
									lResult = RegSetValueExW(hKey,
										NULL,
										0,
										REG_SZ,
										(LPBYTE)szData,
										(lstrlenW(szData) + 1) * sizeof(WCHAR));

									hr = S_OK;
								}

								RegCloseKey(hKey);
							}
						}
					}

					// The Shell has to be notified that the change has been made.
					RefreshFolderViews(CSIDL_DRIVES);
				}
			}
		}
	}

	if (FAILED(hr))
	{
		//Remove the stuff we added.
		DllUnregisterServer();
	}

	return hr;
}


//
// Registry keys are removed here.
//
STDAPI DllUnregisterServer()
{
	WCHAR szSubKey[MAX_PATH], szFolderClassID[MAX_PATH], szElementID[MAX_PATH];

	//Delete the element provider
	HRESULT hrCM = StringFromGUID2(CLSID_FolderViewImplElement,
		szElementID,
		ARRAYSIZE(szElementID));
	if (SUCCEEDED(hrCM))
	{
		hrCM = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), L"CLSID\\%s", szElementID);
		if (SUCCEEDED(hrCM))
		{
			hrCM = SHDeleteKeyW(HKEY_CLASSES_ROOT, szSubKey);
		}
	}

	// Delete the namespace extension entries
	HRESULT hrSF = StringFromGUID2(CLSID_FolderViewImpl, szFolderClassID, ARRAYSIZE(szFolderClassID));
	if (SUCCEEDED(hrSF))
	{
		hrSF = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), CONTROLPANEL_NAMESPACE_GUID, szFolderClassID);
		if (SUCCEEDED(hrSF))
		{
			hrSF = SHDeleteKeyW(HKEY_LOCAL_MACHINE, szSubKey);
			if (SUCCEEDED(hrSF))
			{
				// Delete the object's registry entries
				hrSF = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), L"CLSID\\%s", szFolderClassID);
				if (SUCCEEDED(hrSF))
				{
					hrSF = SHDeleteKeyW(HKEY_CLASSES_ROOT, szSubKey);
					if (SUCCEEDED(hrSF))
					{
						hrSF = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), SHELL_EXT_APPROVED, szFolderClassID);
						if (SUCCEEDED(hrSF))
						{
							hrSF = SHDeleteKeyW(HKEY_LOCAL_MACHINE, szSubKey);

							// Refresh the folder views that might be open
							RefreshFolderViews(CSIDL_DRIVES);
						}
					}
				}
			}
		}
	}

	//Delete the approved key
	HRESULT hr = StringCchPrintfW(szSubKey, ARRAYSIZE(szSubKey), SHELL_EXT_APPROVED, szFolderClassID);
	LSTATUS lResult;
	HKEY hKey;
	if (SUCCEEDED(hr))
	{
		lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &hKey, NULL);
		if (S_OK == lResult)
		{
			lResult = RegDeleteValueW(hKey, szFolderClassID);

			hr = S_OK;

			RegCloseKey(hKey);
		}
	}

	return (SUCCEEDED(hrCM) && SUCCEEDED(hrSF)) ? S_OK : SELFREG_E_CLASS;
}