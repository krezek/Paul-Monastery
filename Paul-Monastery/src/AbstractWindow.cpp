#include "pch.h"
#include "platform.h"

#include <AbstractWindow.h>
#include <resource.h>
#include <WUtil.h>
#include <d3dUtil.h>

AbstractWindow::~AbstractWindow()
{
	if (_d3dDevice)
		FlushCommandQueue();
}

ATOM AbstractWindow::RegisterClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = AbstractWindow::WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_GAME3D));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = AbstractWindow::ClassName();
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_GAME3D));

	return ::RegisterClassEx(&wcex);
}


LRESULT AbstractWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:
		return OnActivate(LOWORD(wParam) != WA_INACTIVE);

	case WM_CREATE:
		return OnCreate();

	case WM_DESTROY:
		return OnDestroy();

	case WM_CLOSE:
		return OnClose();

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
			return 0;

		if (!IsWindowVisible(_hWnd))
			return 0;

		_ClientWidth = LOWORD(lParam);
		_ClientHeight = HIWORD(lParam);

		LRESULT lr = 0;
		
		try
		{
			lr = OnResize();
		}
		catch (const DxException& ex)
		{
			MessageBox(Window(), ex.toString().c_str(), L"HR Failed", MB_OK);
			ExitProcess(-1);
		}

		return lr;
	}

	//case WM_PAINT:
	//	return OnPaint();

	case WM_SIZING:
		return OnSizing((RECT*)lParam);

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		return OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		return OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	case WM_MOUSEMOVE:
		return OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	case WM_KEYDOWN:
		return OnKeyDown(wParam, lParam);

	case WM_CHAR:
		return OnChar(wParam, lParam);

	case WM_DICE_P1_MSG:
		OnDiceP1Msg();
		return 0;

	case WM_DICE_P2_MSG:
		OnDiceP2Msg();
		return 0;

	case WM_TIMER:
	{
		switch (wParam)
		{
		case IDT_TIMER_UP:
			return OnTimer_Up();
		case IDT_TIMER_DOWN:
			return OnTimer_Down();
		case IDT_TIMER_LEFT:
			return OnTimer_Left();
		case IDT_TIMER_RIGHT:
			return OnTimer_Right();
		case IDT_TIMER_IN:
			return OnTimer_Zoomin();
		case IDT_TIMER_OUT:
			return OnTimer_Zoomout();
		case IDT_TIMER_ANIMATE:
			return OnTimer_Animate();
		}
		return 0;
	}
		
	default:
		return DefWindowProc(_hWnd, uMsg, wParam, lParam);
	}
}

LRESULT AbstractWindow::OnPaint()
{
	return 0;
}

LRESULT AbstractWindow::OnActivate(bool active)
{
	if (!active)
	{
		_paused = true;
		_game_timer.Stop();
	}
	else
	{
		_paused = false;
		_game_timer.Start();
	}

	return 0;
}

LRESULT AbstractWindow::OnDestroy()
{
	PostQuitMessage(0);
	return 0;
}

LRESULT AbstractWindow::OnResize()
{
	assert(_d3dDevice);
	assert(_SwapChain);
	assert(_DirectCmdListAlloc);

	FlushCommandQueue();

	ThrowIfFailed(_CommandList->Reset(_DirectCmdListAlloc.Get(), nullptr));

	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		_d2dRenderTargets[i].Reset();
		_wrappedBackBuffers[i].Reset();
		_SwapChainBuffer[i].Reset();
	}
	_DepthStencilBuffer.Reset();

	_d3d11DeviceContext->Flush();

	ThrowIfFailed(_SwapChain->ResizeBuffers(
		SwapChainBufferCount,
		_ClientWidth, _ClientHeight,
		_BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	_CurrBackBuffer = 0;

	float dpiX;
	float dpiY;
#pragma warning(push)
#pragma warning(disable : 4996)
	_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
#pragma warning(pop)
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(_RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(_SwapChain->GetBuffer(i, IID_PPV_ARGS(&_SwapChainBuffer[i])));
		_d3dDevice->CreateRenderTargetView(_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		ThrowIfFailed(_d3d11On12Device->CreateWrappedResource(
			_SwapChainBuffer[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&_wrappedBackBuffers[i])
		));

		Microsoft::WRL::ComPtr<IDXGISurface> surface;
		ThrowIfFailed(_wrappedBackBuffers[i].As(&surface));

		ThrowIfFailed(_d2dDeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			&_d2dRenderTargets[i]
		));

		rtvHeapHandle.Offset(1, _RtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = _ClientWidth;
	depthStencilDesc.Height = _ClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = _DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(_DepthStencilBuffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = _DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	_d3dDevice->CreateDepthStencilView(_DepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	ThrowIfFailed(_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { _CommandList.Get() };
	_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	_ScreenViewport.TopLeftX = 0;
	_ScreenViewport.TopLeftY = 0;
	_ScreenViewport.Width = static_cast<float>(_ClientWidth);
	_ScreenViewport.Height = static_cast<float>(_ClientHeight);
	_ScreenViewport.MinDepth = 0.0f;
	_ScreenViewport.MaxDepth = 1.0f;

	_ScissorRect = { 0, 0, _ClientWidth, _ClientHeight };

	return 0;
}

LRESULT AbstractWindow::OnSizing(RECT* pRect)
{
	LONG nWidth = pRect->right - pRect->left;
	LONG nHeight = pRect->bottom - pRect->top;

	if ((nWidth < 600) || (nHeight < 500))
	{
		pRect->right = max(pRect->right, pRect->left + 600);
		pRect->bottom = max(pRect->bottom, pRect->top + 500);
	}

	return TRUE;
}

LRESULT AbstractWindow::OnClose()
{
	DestroyWindow(_hWnd);
	return 0;
}

LRESULT AbstractWindow::OnMouseDown(WPARAM btnState, int x, int y)
{
	_LastMousePos.x = x;
	_LastMousePos.y = y;

	SetCapture(_hWnd);

	return 0;
}

LRESULT AbstractWindow::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();

	return 0;
}

LRESULT AbstractWindow::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_F1:
		return OnKeyDown_f1();

	case VK_RETURN:
		return OnKeyDown_return();
	break;

	case VK_HOME:       // Home 
		break;

	//case VK_END:        // End 
	//	return OnKeyDown_end();

	case VK_PRIOR:      // Page Up 
		break;

	case VK_NEXT:       // Page Down 
		break;

	case VK_LEFT:       // Left arrow 
		return OnKeyDown_left_arrow();

	case VK_RIGHT:      // Right arrow
		return OnKeyDown_right_arrow();

	case VK_UP:         // Up arrow 
		break;

	case VK_DOWN:       // Down arrow 
		break;

	case VK_DELETE:     // Delete 
		break;
	}

	return 0;
}

LRESULT AbstractWindow::OnChar(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_BACK:          // Backspace 
		break;

	case VK_TAB:          // Tab 
		break;

	case VK_RETURN:          // Carriage return 
		break;

	case VK_ESCAPE:        // Escape 
	case 0x0A:        // Linefeed 
		MessageBeep((UINT)-1);
		break;

	case VK_SPACE:
		return OnChar_space();

	default:
		break;
	}

	return 0;
}

void AbstractWindow::InitDirect3D()
{
#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory)));
	//ThrowIfFailed(_dxgiFactory->MakeWindowAssociation(Window(), DXGI_MWA_NO_ALT_ENTER));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&_d3dDevice));

	if (FAILED(hardwareResult))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&_d3dDevice)));
	}

	ThrowIfFailed(_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&_Fence)));

	_RtvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_DsvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_CbvSrvUavDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = _BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(_d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	_4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();
	Create11On12Objects();
}

void AbstractWindow::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_CommandQueue)));

	ThrowIfFailed(_d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_DirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_DirectCmdListAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(_CommandList.GetAddressOf())));

	_CommandList->Close();
}

void AbstractWindow::CreateSwapChain()
{
	_SwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = _ClientWidth;
	sd.BufferDesc.Height = _ClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = _BackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = Window();
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(_dxgiFactory->CreateSwapChain(
		_CommandQueue.Get(),
		&sd,
		_SwapChain.GetAddressOf()));
}

void AbstractWindow::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(_RtvHeap.GetAddressOf())));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(_DsvHeap.GetAddressOf())));
}

void AbstractWindow::Create11On12Objects()
{
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

	Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device;
	ThrowIfFailed(D3D11On12CreateDevice(
		_d3dDevice.Get(),
		d3d11DeviceFlags,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(_CommandQueue.GetAddressOf()),
		1,
		0,
		&d3d11Device,
		&_d3d11DeviceContext,
		nullptr
	));

	ThrowIfFailed(d3d11Device.As(&_d3d11On12Device));

	{
		D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
		ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &_d2dFactory));
		Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(_d3d11On12Device.As(&dxgiDevice));
		ThrowIfFailed(_d2dFactory->CreateDevice(dxgiDevice.Get(), &_d2dDevice));
		ThrowIfFailed(_d2dDevice->CreateDeviceContext(deviceOptions, &_d2dDeviceContext));
		ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &_dWriteFactory));
	}

	{
		ThrowIfFailed(_d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &_textBrush));
		ThrowIfFailed(_dWriteFactory->CreateTextFormat(
			L"Verdana",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			40,
			L"en-us",
			&_textFormat
		));
		ThrowIfFailed(_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
		ThrowIfFailed(_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
	}
}

void AbstractWindow::FlushCommandQueue()
{
	_CurrentFence++;

	ThrowIfFailed(_CommandQueue->Signal(_Fence.Get(), _CurrentFence));

	if (_Fence->GetCompletedValue() < _CurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(_Fence->SetEventOnCompletion(_CurrentFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE AbstractWindow::DepthStencilView()const
{
	return _DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* AbstractWindow::CurrentBackBuffer() const
{
	return _SwapChainBuffer[_CurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE AbstractWindow::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
		_CurrBackBuffer,
		_RtvDescriptorSize);
}

float AbstractWindow::AspectRatio() const
{
	return static_cast<float>(_ClientWidth) / _ClientHeight;
}

void AbstractWindow::RenderUI()
{
	D2D1_SIZE_F rtSize = _d2dRenderTargets[_CurrBackBuffer]->GetSize();
	D2D1_RECT_F textRect = D2D1::RectF(0, 0, rtSize.width, 200);

	_d3d11On12Device->AcquireWrappedResources(_wrappedBackBuffers[_CurrBackBuffer].GetAddressOf(), 1);

	_d2dDeviceContext->SetTarget(_d2dRenderTargets[_CurrBackBuffer].Get());
	_d2dDeviceContext->BeginDraw();
	_d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
	_d2dDeviceContext->DrawText(
		_text_message.c_str(),
		(UINT32)_text_message.length(),
		_textFormat.Get(),
		&textRect,
		_textBrush.Get()
	);
	ThrowIfFailed(_d2dDeviceContext->EndDraw());

	_d3d11On12Device->ReleaseWrappedResources(_wrappedBackBuffers[_CurrBackBuffer].GetAddressOf(), 1);

	_d3d11DeviceContext->Flush();
	_d2dDeviceContext->SetTarget(nullptr);
}

void AbstractWindow::SetTextMessage(std::wstring& msg)
{
	_text_message = msg;

#ifdef _DEBUG
	wprintf(L"%s\n", _text_message.c_str());
#endif
}

void AbstractWindow::Draw()
{

}

void AbstractWindow::Update()
{

}
