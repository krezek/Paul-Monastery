#include "pch.h"
#include "platform.h"

#include <WUtil.h>

VOID ShowError(LPCTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

VOID RedirectIOToConsole()
{
	int hConHandle;
	HANDLE hStdHandle;
	FILE* fp;

	AllocConsole();

	hStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)hStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	freopen_s(&fp, "CONOUT$", "w", stdout);

	hStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)hStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "r");
	freopen_s(&fp, "CONIN$", "r", stdin);

	hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)hStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	freopen_s(&fp, "CONOUT$", "w", stderr);

	std::ios::sync_with_stdio();
}
