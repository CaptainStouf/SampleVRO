// DrawHelper.cpp
//
// This file contains code to draw to the screen (via a swapchain in D3D), specifically a green ellipse as
// well as any text typed by the user.
//
// Code in this file is based upon the following samples
// - https://docs.microsoft.com/en-us/windows/desktop/Direct2D/how-to--draw-text
// - https://docs.microsoft.com/en-us/windows/desktop/direct2d/direct2d-and-direct3d-interoperation-overview

#include "stdafx.h"
#include "DrawHelper.h"

#include "OpenVRHelper.h"
#include <wincodec.h>
#include "dxtk/Inc/ScreenGrab.h"

DrawHelper::DrawHelper():
	pFactory(nullptr),
	pDevice2d(nullptr),
	pDevice2dContext(nullptr),
	pRenderTarget(nullptr),
	pRenderTargetBitmap(nullptr),
	pRenderTargetHwnd(nullptr),
	pBrush(nullptr),
	pBrushText(nullptr),
	pDevice3d(nullptr),
	pDevice3dContext(nullptr),
	pTex(nullptr),
	pDxgiDevice(nullptr),
	pSurface(nullptr),
	pSwapChain(nullptr),
	pDWriteFactory(nullptr),
	pTextFormat(nullptr)
{
}

DrawHelper::~DrawHelper()
{
}

// Per-instance
HRESULT DrawHelper::Setup()
{
	D2D1_FACTORY_OPTIONS options;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	return D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		&pFactory
	);
}

// Per-instance
void DrawHelper::Shutdown()
{
	DiscardGraphicsResources();
	SafeRelease(&pFactory);
	SafeRelease(&pDevice2d);
	SafeRelease(&pDevice2dContext);
	SafeRelease(&pDevice3d);
	SafeRelease(&pDevice3dContext);
	SafeRelease(&pDxgiDevice);
}

void DrawHelper::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pRenderTargetBitmap);
	SafeRelease(&pRenderTargetHwnd);
	SafeRelease(&pBrush);
	SafeRelease(&pBrushText);
	SafeRelease(&pTex);
	SafeRelease(&pSurface);
	SafeRelease(&pSwapChain);
	SafeRelease(&pDWriteFactory);
	SafeRelease(&pTextFormat);
}

HRESULT DrawHelper::CreateD3DResources(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

	D3D_FEATURE_LEVEL levels [] = { D3D_FEATURE_LEVEL_11_1 };
	D3D_FEATURE_LEVEL deviceLevel = D3D_FEATURE_LEVEL_11_1;

	DXGI_SWAP_CHAIN_DESC descSwapChain = { 0 };
	descSwapChain.BufferDesc.Width = size.width;
	descSwapChain.BufferDesc.Height = size.height;
	descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferDesc.RefreshRate.Numerator = 60;
	descSwapChain.BufferDesc.RefreshRate.Denominator = 1;
	descSwapChain.SampleDesc.Count = 1;
	descSwapChain.SampleDesc.Quality = 0;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.BufferCount = 1;
	descSwapChain.OutputWindow = hwnd;
	descSwapChain.Windowed = TRUE;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr, //pAdapter,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr, // Software
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&descSwapChain,
		&pSwapChain,
		&pDevice3d,
		&deviceLevel,
		&pDevice3dContext
	);

	if (SUCCEEDED(hr))
	{
		// Get a DXGI device interface from the D3D device.
		hr = pDevice3d->QueryInterface(&pDxgiDevice);
		if (hr == S_OK)
		{
			hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pSurface));
			if (hr == S_OK)
			{
				hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pTex));
			}
		}
	}

	return hr;
}

HRESULT DrawHelper::CreateD2DResources()
{
	// Create a D2D device from the DXGI device.
	HRESULT hr = pFactory->CreateDevice(
		pDxgiDevice,
		&pDevice2d
	);

	if (SUCCEEDED(hr))
	{
		// Create a device context from the D2D device.
		hr = pDevice2d->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&pDevice2dContext
		);
	}
	
	return hr;
}

HRESULT DrawHelper::CreateDWriteResources()
{
	// Create a DirectWrite factory.
	HRESULT hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(pDWriteFactory),
		reinterpret_cast<IUnknown **>(&pDWriteFactory)
	);

	if (SUCCEEDED(hr))
	{
		static const WCHAR msc_fontName[] = L"Verdana";
		static const FLOAT msc_fontSize = 50;
		// Create a DirectWrite text format object.
		hr = pDWriteFactory->CreateTextFormat(
			msc_fontName,
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			msc_fontSize,
			L"", //locale
			&pTextFormat
		);
	}

	return hr;
}

// 
HRESULT DrawHelper::CreateGraphicsResources(HWND hwnd, OpenVRHelper* povrHelper)
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		hr = CreateD3DResources(hwnd);
		if (hr == S_OK)
		{
			hr = CreateDWriteResources();
			if (hr == S_OK)
			{
				D2D1_RENDER_TARGET_PROPERTIES props =
					D2D1::RenderTargetProperties(
						D2D1_RENDER_TARGET_TYPE_HARDWARE,
						D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
						96,
						96
					);

				hr = pFactory->CreateDxgiSurfaceRenderTarget(
					pSurface,
					&props,
					&pRenderTarget
				);

				if (hr == S_OK)
				{
					D2D1_BITMAP_PROPERTIES1 bitmapProps =
						D2D1::BitmapProperties1(
							D2D1_BITMAP_OPTIONS_TARGET,
							D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
							0.0f,
							0.0f,
							nullptr // colorContext
						);

					hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0.7f, 0), &pBrush);
					if (SUCCEEDED(hr))
					{
						hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0, 0), &pBrushText);
						if (SUCCEEDED(hr))
						{
							CalculateLayout();
							povrHelper->Init(pTex);
						}
					}
				}
			}			
		}
	}

	return hr;
}

// Recalculate drawing layout when the size of the window changes.
void DrawHelper::CalculateLayout()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		const float x = size.width / 2;
		const float y = size.height / 2;
		const float radius = min(x, y);
		ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
	}
}

void DrawHelper::Draw(HWND hwnd,  OpenVRHelper* povrHelper, WCHAR* pchTypeBuffer, UINT cchTypeBuffer)
{
	HRESULT hr = CreateGraphicsResources(hwnd, povrHelper);
	if (SUCCEEDED(hr))
	{
		pRenderTarget->BeginDraw();

		pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::LightGray) );
		pRenderTarget->FillEllipse(ellipse, pBrush);

		D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();
		pRenderTarget->DrawText(
			pchTypeBuffer,
			cchTypeBuffer,
			pTextFormat,
			D2D1::RectF(0, 0, renderTargetSize.width, renderTargetSize.height),
			pBrushText
		);

		hr = pRenderTarget->EndDraw();

		if (hr == S_OK)
		{
			hr = pSwapChain->Present(0, 0);
			if (hr == S_OK)
			{
				//ID3D11Texture2D* pTex = nullptr;
				//hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pTex));
				if (hr == S_OK)
				{
					povrHelper->SetOverlayTexture(pTex);
					Save(pDevice3dContext, pTex);
				}
			}
		}
		else
		{
			DiscardGraphicsResources();
		}
	}
}

// Save the texture to disk
void DrawHelper::Save(ID3D11DeviceContext* pContext, ID3D11Texture2D* pTex)
{
	// https://github.com/Microsoft/DirectXTK
	DirectX::SaveWICTextureToFile(
		pContext,
		pTex,
		GUID_ContainerFormatBmp,
		L"SampleVRO.bmp"
	);

	//vr::VREvent_t vrEvent;
	//vr::VROverlay()->PollNextOverlayEvent(m_ulOverlayHandle, &vrEvent, sizeof(vrEvent));

	//_RPTF1(_CRT_WARN, "VREvent %d\n", vrEvent.eventType);
}
