#include "pch.h"
#include "platform.h"


//#include <MainWindow.h>
#include <WUtil.h>
//#include <GameTimer.h>
//#include <d3dUtil.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	RedirectIOToConsole();
#endif

	/*if (!MainWindow::RegisterClass())
	{
		ShowError(_T("MainWindow::RegisterClass()"));
		return -1;
	}

	MainWindow win;

	if (!win.Create(_T("Parlor-Board Game3DX"), 
		WS_OVERLAPPEDWINDOW))
	{
		ShowError(_T("MainWindow::Create"));
		return -1;
	}

	try
	{
		win.InitDirect3D();
	}
	catch (const DxException& ex)
	{
		MessageBox(win.Window(), ex.toString().c_str(), L"HR Failed", MB_OK);
		return -1;
	}

	ShowWindow(win.Window(), nCmdShow);
	UpdateWindow(win.Window());

	win.GetGameTimer().Reset(); 
	win.GetGameTimer().Tick();

	try
	{
		win.Update();
		win.Draw();
	}
	catch (const DxException& ex)
	{
		MessageBox(win.Window(), ex.toString().c_str(), L"HR Failed", MB_OK);
		return -1;
	}

	MSG msg = { 0 };

	win.GetGameTimer().Reset();
	
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			win.GetGameTimer().Tick();

			if (!win.isPaused())
			{
				try
				{
					win.Update();
					win.Draw();
				}
				catch (const DxException& ex)
				{
					MessageBox(win.Window(), ex.toString().c_str(), L"HR Failed", MB_OK);
					return -1;
				}
			}
			else
			{
				Sleep(50);
			}
		}
	}

	return (int)msg.wParam;*/

	return 0;
}
