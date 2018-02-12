#pragma once
class Window
{	
public:
	Window();

	static LRESULT CALLBACK    s_WndProc(HWND, UINT, WPARAM, LPARAM);
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, HINSTANCE& g_hInst, HWND& g_hWnd);

private:
	
};

