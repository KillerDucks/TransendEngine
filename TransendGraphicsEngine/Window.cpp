#include "stdafx.h"
#include "Window.h"
#include "resource.h"
#include "DXWndClass.h"
#include "Keyboard.h"
#include <windows.h>

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------
Keyboard			kBoard;

Window::Window()
{
	// No need for constructor
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT Window::InitWindow(HINSTANCE hInstance, int nCmdShow, HINSTANCE& g_hInst, HWND& g_hWnd)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = Window::s_WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TRANSENDGRAPHICSENGINE);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TransendEngineWindow";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TRANSENDGRAPHICSENGINE);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TransendEngineWindow", L"TransendEngine Window ~ DX11",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK Window::s_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_KEYDOWN:
		// Let the Keyboard Class Handle this
		kBoard.KeyDown(wParam);
		break;

	case WM_CREATE:
		// On Window Creation Allocate a Console Window
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

		printf_s("Window Created\n");
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		// Note that this tutorial does not handle resizing (WM_SIZE) requests,
		// so we created the window without the resize border.

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

