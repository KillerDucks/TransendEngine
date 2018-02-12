#include "Keyboard.h"
#include "DXWndClass.h"
#include "stdafx.h"

XMFLOAT3 baseFloat = { 0.0f, 1.0f, -5.0f };


Keyboard::Keyboard()
{
}


Keyboard::~Keyboard()
{
}

void Keyboard::KeyDown(WPARAM key)
{
	// Get Camera Position
	DXWndClass dxClass;
	XMFLOAT3 oldPos = dxClass.GetCameraPos();
	XMFLOAT3 tmpFloat = baseFloat;



	// Set LastKey
	LastKey = key;

	// Temp
	switch (key)
	{
	case VK_ESCAPE:
		//PostMessage(hwnd, WM_DESTROY, 0, 0);
		break;

		// Arrow Keys
	case VK_UP:
		printf_s("Key Pressed -> Up\n");
		
		baseFloat.z = baseFloat.z + 0.1f;
		//XMFLOAT3 tmpFloat = baseFloat;

		dxClass.UpdateCamera(tmpFloat);
		break;
	case VK_LEFT:
		printf_s("Key Pressed -> Left\n");
		
		baseFloat.x = baseFloat.x - 0.1f;
		//XMFLOAT3 tmpFloat = baseFloat;
		dxClass.UpdateCamera(tmpFloat);
		break;
	case VK_RIGHT:
		printf_s("Key Pressed -> Right\n");
		
		baseFloat.x = baseFloat.x + 0.1f;
		

		dxClass.UpdateCamera(tmpFloat);
		break;
	case VK_DOWN:
		printf_s("Key Pressed -> Down\n");

		baseFloat.z = baseFloat.z - 0.1f;
		dxClass.UpdateCamera(tmpFloat);
		break;

	case 0x57:
		// W Key
		printf_s("Key Pressed -> W\n");

		baseFloat.y = baseFloat.y + 0.1f;
		dxClass.UpdateCamera(tmpFloat);
		break;

	case 0x53:
		// S Key
		printf_s("Key Pressed -> W\n");

		baseFloat.y = baseFloat.y - 0.1f;
		dxClass.UpdateCamera(tmpFloat);
		break;

		// Chars (No ALTS ~ Shifted)
	default:
		UINT c = MapVirtualKey(key, MAPVK_VK_TO_CHAR);
		printf_s("Key Pressed -> %c\n", c);
		break;
	}

	//dxClass.UpdateCamera();
}
