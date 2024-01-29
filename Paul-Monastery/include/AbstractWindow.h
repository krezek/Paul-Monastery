#ifndef _ABSTRACT_WINDOW_H_
#define _ABSTRACT_WINDOW_H_

#include <BaseWindow.hpp>
#include <GameTimer.h>

#define WM_DICE_P1_MSG WM_USER + 1
#define WM_DICE_P2_MSG WM_USER + 2

#define TIMER_PERIOD 100
#define TIMER_ANIMATION_PERIOD 15000

#define IDT_TIMER_UP	1000
#define IDT_TIMER_DOWN	1001
#define IDT_TIMER_LEFT	1002
#define IDT_TIMER_RIGHT 1003
#define IDT_TIMER_IN	1004
#define IDT_TIMER_OUT	1005

#define IDT_TIMER_ANIMATE 1100

class AbstractWindow : public BaseWindow<AbstractWindow>
{
	friend static LRESULT CALLBACK BaseWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	AbstractWindow() = default;
	virtual ~AbstractWindow();

	static ATOM RegisterClass();

	virtual LRESULT OnActivate(bool active);
	virtual LRESULT OnCreate() = 0;
	virtual LRESULT OnDestroy();
	virtual LRESULT OnResize();
	virtual LRESULT OnPaint();
	virtual LRESULT OnSizing(RECT*);
	virtual LRESULT OnClose();
	virtual LRESULT OnMouseDown(WPARAM btnState, int x, int y);
	virtual LRESULT OnMouseUp(WPARAM btnState, int x, int y);
	virtual LRESULT OnMouseMove(WPARAM btnState, int x, int y) = 0;
	virtual LRESULT OnKeyDown(WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnChar(WPARAM wParam, LPARAM lParam);
	
	virtual LRESULT OnKeyDown_return() = 0;
	virtual LRESULT OnKeyDown_left_arrow() = 0;
	virtual LRESULT OnKeyDown_right_arrow() = 0;
	virtual LRESULT OnKeyDown_f1() = 0;
	virtual LRESULT OnChar_space() = 0;

	virtual LRESULT OnTimer_Up() = 0;
	virtual LRESULT OnTimer_Down() = 0;
	virtual LRESULT OnTimer_Left() = 0;
	virtual LRESULT OnTimer_Right() = 0;
	virtual LRESULT OnTimer_Zoomin() = 0;
	virtual LRESULT OnTimer_Zoomout() = 0;

	virtual LRESULT OnTimer_Animate() = 0;

	virtual void OnDiceP1Msg() = 0;
	virtual void OnDiceP2Msg() = 0;

	bool isPaused() { return _paused; }

public:
	inline GameTimer& GetGameTimer() { return _game_timer; }
	void InitDirect3D();

	void Draw();
	void Update();

protected:
	static inline PCTSTR ClassName() { return _T("Game Window Class"); }
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	int _ClientWidth;
	int _ClientHeight;

	bool _paused = false;
	GameTimer _game_timer;

	std::wstring _text_message = L"Parlor-Board Game3DX";

	POINT _LastMousePos;

	bool _4xMsaaState = false;
	UINT _4xMsaaQuality = 0; 
	
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> _d3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> _Fence;
	UINT64 _CurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _DirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _CommandList;

	Microsoft::WRL::ComPtr<IDXGISwapChain> _SwapChain;
	static const int SwapChainBufferCount = 2;
	int _CurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> _SwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> _DepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _RtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _DsvHeap;

	D3D12_VIEWPORT _ScreenViewport;
	D3D12_RECT _ScissorRect;

	UINT _RtvDescriptorSize = 0;
	UINT _DsvDescriptorSize = 0;
	UINT _CbvSrvUavDescriptorSize = 0;

	DXGI_FORMAT _BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT _DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3d11DeviceContext;
	Microsoft::WRL::ComPtr<ID3D11On12Device> _d3d11On12Device;
	Microsoft::WRL::ComPtr<IDWriteFactory> _dWriteFactory;
	Microsoft::WRL::ComPtr<ID2D1Factory3> _d2dFactory;
	Microsoft::WRL::ComPtr<ID2D1Device2> _d2dDevice;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext2> _d2dDeviceContext;
	Microsoft::WRL::ComPtr<ID3D11Resource> _wrappedBackBuffers[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> _d2dRenderTargets[SwapChainBufferCount];

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _textBrush;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> _textFormat;

protected:
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();
	void Create11On12Objects();
	void FlushCommandQueue();
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;

	virtual void RenderUI();

	float AspectRatio() const;

	void SetTextMessage(std::wstring& msg);
};

#endif /* _ABSTRACT_WINDOW_H_ */
