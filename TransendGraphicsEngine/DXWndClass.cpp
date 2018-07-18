#include "stdafx.h"
#include "DXWndClass.h"
#include "Window.h"
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "DirectXHelpers.h"
#include "resource.h"

#include "GCamera.h"
#include "Keyboard.h"

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace Game;

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*        g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pVertexLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;
ID3D11Buffer*           g_pIndexBuffer = nullptr;
ID3D11Buffer*           g_pConstantBuffer = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_World1;
XMMATRIX                g_View;
XMMATRIX                g_Projection;

// Class Inits
GCamera					camera;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
};


DXWndClass::DXWndClass()
{
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT DXWndClass::CompileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT DXWndClass::InitDevice(HWND& g_hWnd)
{
	HRESULT hr = S_OK;
	
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = this->CompileShaderFromFile( L"TransendEngine.fx", "VS", "vs_4_0", &pVSBlob );
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	//// Set the input layout
	//g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"TransendEngine.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	//// Set vertex buffer
	//UINT stride = sizeof(SimpleVertex);
	//UINT offset = 0;
	//g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Create index buffer
	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		0,5,4,
		1,5,0,

		3,4,7,
		0,4,3,

		1,6,5,
		2,6,1,

		2,7,6,
		3,7,2,

		6,4,5,
		7,4,6,
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 36;        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	//// Set index buffer
	//g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	//// Set primitive topology
	//g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrix
	g_World = XMMatrixIdentity();
	g_World1 = XMMatrixIdentity();

	// Initialize the view matrix

	//// camera initialization
	//camera.Rotate(XMFLOAT3(0.0f, 0.0f, 1.0f), 90.0f); // roll by 90 degrees clockwise
	camera.Move(camPos);

	XMVECTOR Eye = XMVectorSet(camera.Position().x, camera.Position().y, camera.Position().z, 0.0f);
	XMVECTOR At = XMVectorSet(camera.Target().x, camera.Target().y, camera.Target().z, 0.0f);
	XMVECTOR Up = XMVectorSet(camera.Up().x, camera.Up().y, camera.Up().z, 0.0f);

	g_View = XMMatrixLookAtLH(Eye, At, Up);
	////
	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);

	// Text Render Setup
	m_font = std::make_unique<SpriteFont>(g_pd3dDevice, L"ArialFont.spritefont");
	m_spriteBatch = std::make_unique<SpriteBatch>(g_pImmediateContext);

	m_fontPos1.x = 70;
	m_fontPos1.y = 20;

	m_fontPos2.x = 180;
	m_fontPos2.y = 70;

	// Grid Render Setup
	//m_world = DirectX::SimpleMath::Matrix::Identity;
	m_world = XMMatrixIdentity();

	m_effect = std::make_unique<BasicEffect>(g_pd3dDevice);
	m_states = std::make_unique<CommonStates>(g_pd3dDevice);
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(g_pImmediateContext);

	m_view = DirectX::SimpleMath::Matrix::CreateLookAt(DirectX::SimpleMath::Vector3(2.f, 2.f, 2.f),
		DirectX::SimpleMath::Vector3::Zero, DirectX::SimpleMath::Vector3::UnitY);
	m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
		float(width) / float(height), 0.1f, 10.f);

	m_effect->SetView(m_view);
	//g_View = m_view;
	m_effect->SetProjection(m_proj);
	//g_Projection = m_proj;

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void DXWndClass::Render()
{
	HRESULT hr;

	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) / 1000.0f;
	}


//>> Moved from InitDevice

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


	// Set index buffer
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

//>> Moved from InitDevice

//
// Render the Grid
//

	//g_pImmediateContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	//g_pImmediateContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	//g_pImmediateContext->RSSetState(m_states->CullNone());

	//m_effect->SetWorld(m_world);

	//m_effect->Apply(g_pImmediateContext);

	//g_pImmediateContext->IASetInputLayout(m_inputLayout.Get());

	//m_batch->Begin();

	//DirectX::SimpleMath::Vector3 xaxis(2.f, 0.f, 0.f);
	//DirectX::SimpleMath::Vector3 yaxis(0.f, 0.f, 2.f);
	//DirectX::SimpleMath::Vector3 origin = DirectX::SimpleMath::Vector3::Zero;

	//size_t divisions = 20;

	//for (size_t i = 0; i <= divisions; ++i)
	//{
	//	float fPercent = float(i) / float(divisions);
	//	fPercent = (fPercent * 2.0f) - 1.0f;

	//	DirectX::SimpleMath::Vector3 scale = xaxis * fPercent + origin;

	//	VertexPositionColor v1(scale - yaxis, Colors::White);
	//	VertexPositionColor v2(scale + yaxis, Colors::White);
	//	m_batch->DrawLine(v1, v2);
	//}

	//for (size_t i = 0; i <= divisions; i++)
	//{
	//	float fPercent = float(i) / float(divisions);
	//	fPercent = (fPercent * 2.0f) - 1.0f;

	//	DirectX::SimpleMath::Vector3 scale = yaxis * fPercent + origin;

	//	VertexPositionColor v1(scale - xaxis, Colors::White);
	//	VertexPositionColor v2(scale + xaxis, Colors::White);
	//	m_batch->DrawLine(v1, v2);
	//}

	//m_batch->End();

	//printf_s("FPS -> %d \n", this->CalculateFPS());

	//
	// Animate the cube (Cube 0)
	//
	g_World = XMMatrixRotationY(t);

	// Cube (Cube 1):  Rotate around origin
	//XMMATRIX mSpin = XMMatrixRotationZ(t);
	//XMMATRIX mOrbit = XMMatrixRotationY(t * 2.0f);
	//XMMATRIX mTranslate = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);
	//XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);

	float distance = 5.0f;
	XMMATRIX matSpin = XMMatrixRotationZ(-t);
	XMMATRIX matRot = XMMatrixRotationX(distance * cos(90));
	XMMATRIX matOrbit = XMMatrixRotationY(-t * 2.0f);
	XMMATRIX matTrans = XMMatrixTranslation(2.0f, 2.0f, 0.0f);
	XMMATRIX matScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);
	XMMATRIX matFinal = matTrans * matRot * matOrbit * matSpin * matScale;

	g_World1 = matFinal;

	//
	// Clear the back buffer
	//
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::Black);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//
	// Update variables
	//
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(g_World);
	cb.mView = XMMatrixTranspose(g_View);
	cb.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	//
	// Renders a triangle
	//
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->DrawIndexed(36, 0, 0);				// 36 vertices needed for 12 triangles in a triangle list

	
	//
	// Update variables for the second cube
	//
	ConstantBuffer cb2;
	cb2.mWorld = XMMatrixTranspose(g_World1);
	cb2.mView = XMMatrixTranspose(g_View);
	cb2.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb2, 0, 0);

	//
	// Render the second cube
	//
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	//
	// Render Text
	//
	m_spriteBatch->Begin();

	wchar_t m_fpsCount[256];
	swprintf_s(m_fpsCount, L"FPS: %d", CalculateFPS());

	//wchar_t m_XYZ;
	//m_XYZ = (wchar_t)"[X,Y,Z] --> Not yet avaliable";
	//swprintf_s(m_XYZ, L"(X,Y,Z): %0.2f , %0.2f , %0.2f", this->GetXYZ().x, this->GetXYZ().y, this->GetXYZ().z);
	//swprintf_s(m_XYZ, L"(X,Y,Z): %0.2f , %0.2f , %0.2f", GetXYZPos('x'), GetXYZPos('y'), GetXYZPos('z'));
	//swprintf_s(m_XYZ, L"FPS: %d", CalculateFPS());


	const wchar_t* output1 = m_fpsCount;
	//const wchar_t* output2 = (const wchar_t*)m_XYZ;

	DirectX::SimpleMath::Vector2 origin1 = m_font->MeasureString(output1) / 2.f;
	//DirectX::SimpleMath::Vector2 origin2 = m_font->MeasureString(output2) / 2.f;

	m_font->DrawString(m_spriteBatch.get(), output1,
		m_fontPos1, Colors::Red, 0.f, origin1);

	//m_font->DrawString(m_spriteBatch.get(), output2,
	//	m_fontPos2, Colors::Red, 0.f, origin2);

	m_spriteBatch->End();



	//
	// Present our back buffer to our front buffer
	//
	hr = g_pSwapChain->Present(0, 0);

	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Cannot present without error.", L"Error", MB_OK);
	}
}

void DXWndClass::Logic()
{
	// DirectX Logic Here

	

}

void DXWndClass::UpdateCamera(XMFLOAT3 newEy0e)
{
	// Updates the Camera

	printf_s("Now Updating the Camera!\n");

	//camera.Move(this->GetCameraPos());

	XMVECTOR newEye = XMVectorSet(newEy0e.x, newEy0e.y, newEy0e.z, 0.0f);
	XMVECTOR newAt = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR newUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	camXYZPos = newEy0e;

	g_View = XMMatrixLookAtLH(newEye, newAt, newUp);

	// New Camera Pos
	printf_s("New Camera Position (X,Y,Z) --> %0.2f , %0.2f , %0.2f\n", newEy0e.x, newEy0e.y, newEy0e.z);
}

void DXWndClass::SetCameraPos(XMFLOAT3 newCamPos)
{
	camPosMoved = true;
	camPos = newCamPos;
}

//void DXWndClass::SetEyePos(XMVECTOR newEyePos)
//{
//	Eye = newEyePos;
//}
//
//void DXWndClass::SetAtPos(XMVECTOR newAtPos)
//{
//	At = newAtPos;
//}
//
//void DXWndClass::SetUpPos(XMVECTOR newUpPos)
//{
//	Up = newUpPos;
//}

void DXWndClass::SetBufferWidthPos(UINT newWidth)
{
	width = newWidth;
}

void DXWndClass::SetBufferHeight(UINT newHeight)
{
	height = newHeight;
}

//-------------------------------------
// Calculate FPS
//-------------------------------------
int DXWndClass::CalculateFPS()
{
	static int iCurrentTick = 0, iFps = 0, iFrames = 0;
	static int iStartTick = GetTickCount();

	iFrames++;
	iCurrentTick = GetTickCount();
	if ((iCurrentTick - iStartTick) >= 1000)
	{
		iFps = (int)((float)iFrames / (iCurrentTick - iStartTick)*1000.0f);
		iFrames = 0;
		iStartTick = iCurrentTick;
	}
	return iFps;
}

//float DXWndClass::GetXYZPos(char axis)
//{
//	switch (axis)
//	{
//	case 'x':
//		return this->camXYZPos.x;
//		break;
//	case 'y':
//		return this->camXYZPos.y;
//		break;
//	case 'z':
//		return this->camXYZPos.z;
//		break;
//
//	default:
//		return 0.0f;
//		break;
//	}
//}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void DXWndClass::CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext1->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	// Font Clean-up
	m_font.reset();
	m_spriteBatch.reset();
	// Grid Clean-up
	m_states.reset();
	m_effect.reset();
	m_batch.reset();
	m_inputLayout.Reset();
}


