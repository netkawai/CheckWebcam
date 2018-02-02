// CheckWebcam.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//
// ChooseDeviceParam structure
//
// Holds an array of IMFActivate pointers that represent video
// capture devices.
//

struct ChooseDeviceParam
{
	IMFActivate **ppDevices;    // Array of IMFActivate pointers.
	UINT32      count;          // Number of elements in the array.
	UINT32      selection;      // Selected device, by array index.
};

CPreview *g_Preview = NULL;

BOOL InitializeApplication()
{
	HRESULT hr = S_OK;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		hr = MFStartup(MF_VERSION);
	}

	return (SUCCEEDED(hr));
}

//-------------------------------------------------------------------
//  OnChooseDevice
//
//  Select a video capture device.
//
//  hwnd:    A handle to the application window.
/// bPrompt: If TRUE, prompt to user to select the device. Otherwise,
//           select the first device in the list.
//-------------------------------------------------------------------

void OnChooseDevice()
{
	HRESULT hr = S_OK;
	ChooseDeviceParam param = { 0 };

	UINT iDevice = 0;   // Index into the array of devices
	BOOL bCancel = FALSE;

	IMFAttributes *pAttributes = NULL;

	// Initialize an attribute store to specify enumeration parameters.

	hr = MFCreateAttributes(&pAttributes, 1);

	if (FAILED(hr)) { goto done; }

	// Ask for source type = video capture devices.

	hr = pAttributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);

	if (FAILED(hr)) { goto done; }

	// Enumerate devices.
	hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);

	if (FAILED(hr)) { goto done; }

	if (param.count > 0)
	{
		// Give this source to the CPlayer object for preview.
		hr = g_Preview->SetDevice(param.ppDevices[iDevice]);
	}

	// NOTE: param.count might be zero.
done:

	SafeRelease(&pAttributes);

	for (DWORD i = 0; i < param.count; i++)
	{
		SafeRelease(&param.ppDevices[i]);
	}
	CoTaskMemFree(param.ppDevices);

	if (FAILED(hr))
	{
//		ShowErrorMessage(L"Cannot create a video capture device", hr);
	}
}

void CleanUp()
{
	if (g_Preview)
	{
		g_Preview->CloseDevice();
	}

	SafeRelease(&g_Preview);

	MFShutdown();
	CoUninitialize();
}



int OnCreateDevice()
{
	HRESULT hr = CPreview::CreateInstance(&g_Preview);
	OnChooseDevice();

	if (g_Preview->GetStatus() == 0x8007001F)
	{
		return -1;
	}
	return 0;
}


int main()
{
	InitializeApplication();
	int res = OnCreateDevice();
	CleanUp();
    return res;
}

