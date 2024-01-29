#ifndef _BASE_WINDOW_HPP_
#define _BASE_WINDOW_HPP_

#include <wtypes.h>

template<typename DERAIVED_TYPE>
class BaseWindow
{
protected:
	HWND _hWnd;

public:
	BaseWindow();
	virtual ~BaseWindow();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL Create(PCTSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int X = CW_USEDEFAULT,
		int Y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0);

	virtual BOOL MoveWindow(int X, int Y, int nWidth, int nHeight, BOOL bRepaint);

	virtual LRESULT OnCreate() = 0;
	virtual LRESULT OnDestroy() = 0;
	virtual LRESULT OnPaint() = 0;
	virtual LRESULT OnSizing(RECT*) = 0;
	
	HWND Window() const;

	void SetText(LPCTSTR);

protected:
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};

template<typename DERIVED_TYPE>
BaseWindow<DERIVED_TYPE>::BaseWindow()
	:_hWnd(NULL)
{
}

template<typename DERIVED_TYPE>
BaseWindow<DERIVED_TYPE>::~BaseWindow()
{
}

template<typename DERIVED_TYPE>
LRESULT BaseWindow<DERIVED_TYPE>::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DERIVED_TYPE *pThis = NULL;

	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

		pThis->_hWnd = hwnd;
	}
	else
	{
		pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	if (pThis)
	{
		return pThis->HandleMessage(uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

template<typename DERIVED_TYPE>
BOOL BaseWindow<DERIVED_TYPE>::Create(PCTSTR lpWindowName,
	DWORD dwStyle, DWORD dwExStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu)
{
	_hWnd = CreateWindowEx(
		dwExStyle, DERIVED_TYPE::ClassName(), lpWindowName, dwStyle, X, Y,
		nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
	);

	return (_hWnd ? TRUE : FALSE);
}

template<typename DERIVED_TYPE>
inline BOOL BaseWindow<DERIVED_TYPE>::MoveWindow(int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	return ::MoveWindow(_hWnd, X, Y, nWidth, nHeight, bRepaint);
}

template<typename DERIVED_TYPE>
inline HWND BaseWindow<DERIVED_TYPE>::Window() const
{
	return _hWnd;
}

template<typename DERIVED_TYPE>
inline void BaseWindow<DERIVED_TYPE>::SetText(LPCTSTR wstr)
{
	SendMessage(_hWnd, WM_SETTEXT, NULL, (LPARAM)wstr);
}


#endif /* _BASE_WINDOW_HPP_ */
