#pragma once
#include "undoc.h"
class CplProvider : public DirectUI::XProvider, public IDUIElementProviderInit, public IFrameNotificationClient, public IFrameShellViewClient, public IObjectWithSite, public IServiceProvider
{
public:
	CplProvider();

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, __out void** ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	virtual long CreateDUI(DirectUI::IXElementCP* a, HWND* hwnd);

	// IDUIElementProviderInit
	virtual HRESULT STDMETHODCALLTYPE SetResourceID(UINT id);
	virtual HRESULT STDMETHODCALLTYPE OptionallyTakeInitialFocus(BOOL* result);

	//IFrameNotificationClient
	virtual HRESULT STDMETHODCALLTYPE LayoutInitialized();
	virtual HRESULT STDMETHODCALLTYPE Notify(WORD* param);
	virtual HRESULT STDMETHODCALLTYPE OnNavigateAway();
	virtual HRESULT STDMETHODCALLTYPE OnInnerElementDestroyed();

	//IFrameShellViewClient
	virtual HRESULT STDMETHODCALLTYPE OnSelectedItemChanged();
	virtual HRESULT STDMETHODCALLTYPE OnSelectionChanged();
	virtual HRESULT STDMETHODCALLTYPE OnContentsChanged();
	virtual HRESULT STDMETHODCALLTYPE OnFolderChanged();



	//IServiceProvider

	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject);


	//IobjectWithSite
	virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite);

	virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite);

private:
	IUnknown* _punkSite;
	DirectUI::XResourceProvider* resourceProvider = NULL;
	DirectUI::IXProviderCP** provider = NULL;
	~CplProvider();
};
