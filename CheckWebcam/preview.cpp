//////////////////////////////////////////////////////////////////////////
//
// preview.cpp: Manages video preview.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <shlwapi.h>

//-------------------------------------------------------------------
//  CreateInstance
//
//  Static class method to create the CPreview object.
//-------------------------------------------------------------------

HRESULT CPreview::CreateInstance(
    CPreview **ppPlayer // Receives a pointer to the CPreview object.
    )
{

    CPreview *pPlayer = new (std::nothrow) CPreview();

    // The CPlayer constructor sets the ref count to 1.

    if (pPlayer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPlayer->Initialize();

    if (SUCCEEDED(hr))
    {
        *ppPlayer = pPlayer;
        (*ppPlayer)->AddRef();
    }

    SafeRelease(&pPlayer);
    return hr;
}


//-------------------------------------------------------------------
//  constructor
//-------------------------------------------------------------------

CPreview::CPreview() : 
    m_pReader(NULL),
    m_nRefCount(1),
    m_pwszSymbolicLink(NULL),
    m_cchSymbolicLink(0),
	m_hResult(-1)
{
    InitializeCriticalSection(&m_critsec);
	m_waitEvent = CreateEvent(NULL, TRUE, FALSE, _T("WAIT_SAMPLE"));
}

//-------------------------------------------------------------------
//  destructor
//-------------------------------------------------------------------

CPreview::~CPreview()
{
    CloseDevice();
    DeleteCriticalSection(&m_critsec);
	CloseHandle(m_waitEvent);
}


//-------------------------------------------------------------------
//  Initialize
//
//  Initializes the object.
//-------------------------------------------------------------------

HRESULT CPreview::Initialize()
{
    HRESULT hr = S_OK;

    return hr;
}


//-------------------------------------------------------------------
//  CloseDevice
//
//  Releases all resources held by this object.
//-------------------------------------------------------------------

HRESULT CPreview::CloseDevice()
{
    EnterCriticalSection(&m_critsec);

    SafeRelease(&m_pReader);

    CoTaskMemFree(m_pwszSymbolicLink);
    m_pwszSymbolicLink = NULL;
    m_cchSymbolicLink = 0;

    LeaveCriticalSection(&m_critsec);
    return S_OK;
}


/////////////// IUnknown methods ///////////////

//-------------------------------------------------------------------
//  AddRef
//-------------------------------------------------------------------

ULONG CPreview::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}


//-------------------------------------------------------------------
//  Release
//-------------------------------------------------------------------

ULONG CPreview::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    // For thread safety, return a temporary variable.
    return uCount;
}



//-------------------------------------------------------------------
//  QueryInterface
//-------------------------------------------------------------------

HRESULT CPreview::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPreview, IMFSourceReaderCallback),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


/////////////// IMFSourceReaderCallback methods ///////////////

//-------------------------------------------------------------------
// OnReadSample
//
// Called when the IMFMediaSource::ReadSample method completes.
//-------------------------------------------------------------------

HRESULT CPreview::OnReadSample(
    HRESULT hrStatus,
    DWORD /* dwStreamIndex */,
    DWORD /* dwStreamFlags */,
    LONGLONG /* llTimestamp */,
    IMFSample *pSample      // Can be NULL
    )
{
    HRESULT hr = S_OK;
    IMFMediaBuffer *pBuffer = NULL;

    EnterCriticalSection(&m_critsec);

    if (FAILED(hrStatus))
    {
        hr = hrStatus;
    }

    if (SUCCEEDED(hr))
    {
        if (pSample)
        {
            // Get the video frame buffer from the sample.

            hr = pSample->GetBufferByIndex(0, &pBuffer);
        }
    }

    // Request the next frame.
    if (SUCCEEDED(hr))
    {
        hr = m_pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            NULL,   // actual
            NULL,   // flags
            NULL,   // timestamp
            NULL    // sample
            );
    }

	m_hResult = hr;

	SetEvent(m_waitEvent);
    SafeRelease(&pBuffer);
    LeaveCriticalSection(&m_critsec);
    return hr;
}


int CPreview::GetStatus()
{
	WaitForSingleObject(m_waitEvent, 3*1000);
	return m_hResult;
}



//-------------------------------------------------------------------
// SetDevice
//
// Set up preview for a specified video capture device. 
//-------------------------------------------------------------------

HRESULT CPreview::SetDevice(IMFActivate *pActivate)
{
    HRESULT hr = S_OK;

    IMFMediaSource  *pSource = NULL;
    IMFAttributes   *pAttributes = NULL;
    IMFMediaType    *pType = NULL;

    EnterCriticalSection(&m_critsec);

    // Release the current device, if any.

    hr = CloseDevice();

    // Create the media source for the device.
    if (SUCCEEDED(hr))
    {
        hr = pActivate->ActivateObject(
            __uuidof(IMFMediaSource), 
            (void**)&pSource
            );
    }

    // Get the symbolic link.
    if (SUCCEEDED(hr))
    {
        hr = pActivate->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            &m_pwszSymbolicLink,
            &m_cchSymbolicLink
            );
    }


    //
    // Create the source reader.
    //

    // Create an attribute store to hold initialization settings.

    if (SUCCEEDED(hr))
    {
        hr = MFCreateAttributes(&pAttributes, 2);
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
    }

    // Set the callback pointer.
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUnknown(
            MF_SOURCE_READER_ASYNC_CALLBACK,
            this
            );
    }

    if (SUCCEEDED(hr))
    {
        hr = MFCreateSourceReaderFromMediaSource(
            pSource,
            pAttributes,
            &m_pReader
            );
    }

    if (SUCCEEDED(hr))
    {
        // Ask for the first sample.
        hr = m_pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            NULL,
            NULL,
            NULL,
            NULL
            );
    }

    if (FAILED(hr))
    {
        if (pSource)
        {
            pSource->Shutdown();

            // NOTE: The source reader shuts down the media source
            // by default, but we might not have gotten that far.
        }
        CloseDevice();
    }

    SafeRelease(&pSource);
    SafeRelease(&pAttributes);
    SafeRelease(&pType);

    LeaveCriticalSection(&m_critsec);
    return hr;
}
