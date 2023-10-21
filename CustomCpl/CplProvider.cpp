#include "pch.h"
#include "global.h"
#include "CplProvider.h"


#define NOT_IMPLEMENTED MessageBox(NULL, TEXT(__FUNCTION__), TEXT("Non implementented function in CElementProvider"), MB_ICONERROR)
#define SHOW_ERROR(x) MessageBox(NULL, TEXT(x), TEXT("Error in CElementProvider"), MB_ICONERROR)


CplProvider::CplProvider() : _punkSite(NULL)
{
	if (FAILED(InitProcessPriv(14, NULL, 1, true)))
	{
		SHOW_ERROR("Failed to initialize DirectUI\n");
	}
	if (FAILED(InitThread(2)))
	{
		SHOW_ERROR("Failed to initialize DirectUI for thread\n");
	}
	refCount = 1;
	DllAddRef();
}

CplProvider::~CplProvider()
{
	UnInitThread();
	UnInitProcessPriv((unsigned short*)g_hInst);

	DllRelease();
}


HRESULT CplProvider::QueryInterface(REFIID riid, __out void** ppv)
{
	static const QITAB qit[] = {
		QITABENT(CplProvider, IDUIElementProviderInit),
		QITABENT(CplProvider, IFrameNotificationClient),
		QITABENT(CplProvider, IFrameShellViewClient),
		QITABENT(CplProvider, IObjectWithSite),
		QITABENT(CplProvider, IServiceProvider),
		{ 0 },
	};
	HRESULT hr = QISearch(this, qit, riid, ppv);

	if (hr != S_OK)
	{
		hr = DirectUI::XProvider::QueryInterface(riid, ppv);
	}
	if (hr != S_OK)
	{
		WCHAR szGuid[40] = { 0 };

		swprintf_s(szGuid, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

		MessageBox(NULL, szGuid, TEXT("Unknown interface in CplProvider::QueryInterface()"), MB_ICONERROR);
	}
	return hr;
}

ULONG CplProvider::AddRef()
{
	return DirectUI::XProvider::AddRef();
}

ULONG CplProvider::Release()
{
	ULONG cRef = InterlockedDecrement(&refCount);
	if (0 == cRef)
	{
		delete this;
	}
	return cRef;
}

HRESULT CplProvider::CreateDUI(DirectUI::IXElementCP* a, HWND* result_handle)
{
	int hr = XProvider::CreateDUI(a, result_handle);
	if (SUCCEEDED(hr))
	{
		DirectUI::XProvider::SetHandleEnterKey(true);
	}
	else
	{
		WCHAR buffer[200];
		if (hr == 0x800403ED)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Bad markup.");
		}
		else if (hr == 0x800403EF)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: A required property is missing. (are you sure that resid=main exists?)");
		}
		else if (hr == 0x800403F1)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Invaild property value");
		}
		else if (hr == 0x8004005A)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Probaby can't find the UIFILE?");
		}
		else if (hr == 0x800403EE)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Unregistered element");
		}
		else if (hr == 0x800403F0)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Something is not found");
		}
		else if (hr == 0x800403F0)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Something is not found");
		}
		else if (hr == E_FAIL)
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: E_FAIL");
		}
		else
		{
			swprintf(buffer, 200, L"Failed to create DirectUI parser: Error %X", hr);
		}

		MessageBox(NULL, buffer, TEXT("CElementProvider::CreateDUI failed"), MB_ICONERROR);
		return hr;
	}
	return 0;
}
#pragma warning( push )
#pragma warning( disable : 4312 ) // disable warning about compiler complaining about casting ID to pointer
HRESULT STDMETHODCALLTYPE CplProvider::SetResourceID(UINT id)
{
	IFrameShellViewClient* client = this;

	WCHAR buffer[264];
	StringCchCopyW(buffer, 260, L"main");

	//First parmeter: hinstance of module
	//2nd: Resource ID of uifile
	//3rd param: The main resid value
	int hr = DirectUI::XResourceProvider::Create(g_hInst, (UCString)id, (UCString)buffer, 0, &this->resourceProvider);
	if (SUCCEEDED(hr))
	{
		hr = DirectUI::XProvider::Initialize(NULL, (IXProviderCP*)this->resourceProvider);
		if (!SUCCEEDED(hr))
		{
			WCHAR szResource[40] = { 0 };
			swprintf(szResource, 40, L"%d", id);

			MessageBox(NULL, szResource, TEXT("CElementProvider::SetResourceId Failed to initialize xprovider"), MB_ICONERROR);
		}
	}
	else
	{
		WCHAR szResource[40] = { 0 };
		swprintf(szResource, 40, L"%d", id);

		MessageBox(NULL, szResource, TEXT("CElementProvider::SetResourceId failed to create xprovider"), MB_ICONERROR);
	}
	return hr;
}
#pragma warning( pop )

HRESULT STDMETHODCALLTYPE CplProvider::OptionallyTakeInitialFocus(BOOL* result)
{
	*result = 0;
	Element* root = DirectUI::XProvider::GetRoot();
	if (root != NULL)
	{
		//root->GetClassInfoPtr();
		//TODO
	}
	return 0;
}
class EventListener : public IElementListener {

	using handler_t = std::function<void(Element*, Event*)>;

	handler_t f;
public:
	EventListener(handler_t f) : f(f) { }

	void OnListenerAttach(Element* elem) override { }
	void OnListenerDetach(Element* elem) override { }
	bool OnPropertyChanging(Element* elem, const PropertyInfo* prop, int unk, Value* v1, Value* v2) override {
		return true;
	}
	void OnListenedPropertyChanged(Element* elem, const PropertyInfo* prop, int type, Value* v1, Value* v2) override { }
	void OnListenedEvent(Element* elem, struct Event* iev) override {
		f(elem, iev);
	}
	void OnListenedInput(Element* elem, struct InputEvent* ev) override { }
};
//
//void CplProvider::InitNavLinks()
//{
//	auto links = new CControlPanelNavLinks();
//
//	WCHAR buffer[1024];
//	if (FAILED(LoadStringW(g_hInst, IDS_UPDATE, buffer, 1023)))
//	{
//		wcscpy_s(buffer, L"Failed to load localized string");
//	}
//	links->AddLinkControlPanel(buffer, L"Rectify11.SettingsCPL", L"pageThemePref", CPNAV_Normal, NULL);
//	links->AddLinkControlPanel(L"System information", L"Microsoft.System", L"", CPNAV_SeeAlso, NULL);
//
//
//	GUID SID_PerLayoutPropertyBag = {};
//	HRESULT hr = CLSIDFromString(L"{a46e5c25-c09c-4ca8-9a53-49cf7f865525}", (LPCLSID)&SID_PerLayoutPropertyBag);
//	if (SUCCEEDED(hr))
//	{
//		IPropertyBag* bag = NULL;
//		int hr = IUnknown_QueryService(_punkSite, SID_PerLayoutPropertyBag, IID_IPropertyBag, (LPVOID*)&bag);
//		if (SUCCEEDED(hr))
//		{
//			if (SUCCEEDED(PSPropertyBag_WriteUnknown(bag, L"ControlPanelNavLinks", links)))
//			{
//
//			}
//			else {
//				MessageBox(NULL, TEXT("Failed to write property bag for navigation links"), TEXT("CElementProvider::InitNavLinks"), 0);
//			}
//			bag->Release();
//		}
//		else {
//			MessageBox(NULL, TEXT("Failed to get property bag for navigation links"), TEXT("CElementProvider::InitNavLinks"), 0);
//		}
//	}
//	else
//	{
//		MessageBox(NULL, TEXT("Failed to parse hardcoded GUID (SID_PerLayoutPropertyBag)"), TEXT("CElementProvider::InitNavLinks"), 0);
//	}
//}

HRESULT STDMETHODCALLTYPE CplProvider::LayoutInitialized()
{

	return S_OK;
}
HRESULT STDMETHODCALLTYPE CplProvider::Notify(WORD* param)
{
	//the param is text

	if (!StrCmpCW((LPCWSTR)param, L"SettingsChanged"))
	{
		//This is invoked when the UI is refreshed!
	}

	if (!StrCmpCW((LPCWSTR)param, L"SearchText"))
	{
		//Sent when search text modified/added
		WCHAR value[264] = { 0 };
		WCHAR path[48];
		GUID IID_IFrameManager = {};
		GUID SID_STopLevelBrowser = {};
		HRESULT hr = CLSIDFromString(L"{4c96be40-915c-11cf-99d3-00aa004ae837}", (LPCLSID)&SID_STopLevelBrowser);
		if (SUCCEEDED(hr))
		{
			hr = CLSIDFromString(L"{31e4fa78-02b4-419f-9430-7b7585237c77}", (LPCLSID)&IID_IFrameManager);
			if (SUCCEEDED(hr))
			{
				IPropertyBag* bag = NULL;
				HRESULT hr = IUnknown_QueryService(_punkSite, IID_IFrameManager, IID_IPropertyBag, (LPVOID*)&bag);
				if (SUCCEEDED(hr))
				{
					if (SUCCEEDED(PSPropertyBag_ReadStr(bag, L"SearchText", value, 260)) && value[0])
					{
						if (SUCCEEDED(StringCchPrintfW(path, 41, L"::%s", L"{26ee0668-a00a-44d7-9371-beb064c98683}")))
						{
							LPITEMIDLIST pidlist;
							if (SUCCEEDED(SHParseDisplayName(path, NULL, &pidlist, 0, NULL)))
							{
								IShellBrowser* browser = NULL;
								if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (LPVOID*)&browser)))
								{
									hr = browser->BrowseObject(pidlist, SBSP_ACTIVATE_NOFOCUS | SBSP_SAMEBROWSER | SBSP_CREATENOHISTORY);
									browser->Release();
								}
							}
							ILFree(pidlist);
						}
					}
				}
			}
		}
	}
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnNavigateAway() {
	//TODO: this causes a crash
	//DirectUI::XProvider::SetHandleEnterKey(false);
	//SetDefaultButtonTracking(false);
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnInnerElementDestroyed() {
	return 0;
}

HRESULT STDMETHODCALLTYPE CplProvider::OnSelectedItemChanged()
{
	NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnSelectionChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnContentsChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnFolderChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}

HRESULT STDMETHODCALLTYPE CplProvider::QueryService(
	REFGUID guidService,
	REFIID riid,
	void** ppvObject)
{
	*ppvObject = 0;
	NOT_IMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT CplProvider::SetSite(IUnknown* punkSite)
{
	if (punkSite != NULL)
	{
		IUnknown_Set((IUnknown**)&this->Site, punkSite);
		IUnknown_Set((IUnknown**)&this->_punkSite, punkSite);
	}

	return S_OK;
}

HRESULT CplProvider::GetSite(REFIID riid, void** ppvSite)
{
	NOT_IMPLEMENTED;

	if (Site == NULL)
	{
		return E_FAIL;
	}

	return Site->QueryInterface(riid, ppvSite);
}
