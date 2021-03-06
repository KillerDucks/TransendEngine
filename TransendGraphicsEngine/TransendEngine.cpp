// TransendGraphicsEngine.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <windows.h>
#include "TransendEngine.h"
#include "Window.h"
#include "DXWndClass.h"

HINSTANCE				g_hInst = nullptr;
HWND					g_hWnd = nullptr;

Window					wndClass;
DXWndClass				dxClass;

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(wndClass.InitWindow(hInstance, nCmdShow, g_hInst, g_hWnd))) 
	{
		MessageBox(nullptr,
			L"Window Could Not be Created!", L"Error", MB_OK);
		return 0;
	}
		

	if (FAILED(dxClass.InitDevice(g_hWnd)))
	{
		MessageBox(nullptr,
			L"Could not Init DirectX 3D!", L"Error", MB_OK);
		dxClass.CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Handle Game Logic
			//dxClass.Logic();

			// Render Frame to Screen
			dxClass.Render();
		}
	}

	dxClass.CleanupDevice();

	return (int)msg.wParam;
}