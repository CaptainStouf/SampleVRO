#pragma once

#include "OpenVRHelper.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite.h>



class DrawHelper
{
public:
	DrawHelper();
	~DrawHelper();

	HRESULT Setup();	

	void Draw(HWND hwnd,  OpenVRHelper* povrHelper, WCHAR* pchTypeBuffer, UINT cchTypeBuffer);
	void CalculateLayout();
	void Save(ID3D11DeviceContext* pContext, ID3D11Texture2D* pTex);

	void DiscardGraphicsResources();
	void Shutdown();

private:
	HRESULT CreateGraphicsResources(HWND hwnd,  OpenVRHelper* povrHelper);
	HRESULT CreateD3DResources(HWND hwnd);
	HRESULT CreateD2DResources();
	HRESULT CreateDWriteResources();


	ID2D1Factory1           *pFactory;
	ID2D1Device				*pDevice2d;
	ID2D1DeviceContext		*pDevice2dContext;

	ID2D1RenderTarget		*pRenderTarget;
	ID2D1Bitmap1			*pRenderTargetBitmap;
	ID2D1HwndRenderTarget   *pRenderTargetHwnd;
	ID2D1SolidColorBrush    *pBrush;
	ID2D1SolidColorBrush    *pBrushText;
	D2D1_ELLIPSE            ellipse;

	ID3D11Device			*pDevice3d;
	ID3D11DeviceContext		*pDevice3dContext;
	ID3D11Texture2D			*pTex;

	IDXGIDevice				*pDxgiDevice;
	IDXGISurface			*pSurface;
	IDXGISwapChain			*pSwapChain;

	IDWriteFactory			*pDWriteFactory;
	IDWriteTextFormat		*pTextFormat;
};

