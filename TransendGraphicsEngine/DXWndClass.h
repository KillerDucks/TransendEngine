#pragma once

#include <memory>
#include <d3d11_1.h>
#include <directxmath.h>
#include <SpriteFont.h>
#include <SimpleMath.h>
#include <Effects.h>
#include <CommonStates.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <wrl.h>

using namespace DirectX;


class DXWndClass
{
public:
	DXWndClass();

	HRESULT CompileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitDevice(HWND& g_hWnd);
	void CleanupDevice();	
	void Render();
	void Logic();

	void UpdateCamera(XMFLOAT3 newEy0e);

	//----------------------
	// Text/Sprite Rendering Vars
	//----------------------
	std::unique_ptr<DirectX::SpriteFont> m_font;
	DirectX::SimpleMath::Vector2 m_fontPos1;
	DirectX::SimpleMath::Vector2 m_fontPos2;
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

	//----------------------
	// Grid Rendering Vars
	//----------------------
	DirectX::SimpleMath::Matrix m_world;
	DirectX::SimpleMath::Matrix m_view;
	DirectX::SimpleMath::Matrix m_proj;

	std::unique_ptr<DirectX::CommonStates> m_states;
	std::unique_ptr<DirectX::BasicEffect> m_effect;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

	int CalculateFPS();

	float GetXYZPos(char axis);

	// Change + Get Camera Position
	const XMFLOAT3& GetCameraPos() const { return camPos; }
	void SetCameraPos(XMFLOAT3 newCamPos);

	// Getters
	//const XMVECTOR& GetEyePos() const { return Eye; }
	//const XMVECTOR& GetAtPos() const { return At; }
	//const XMVECTOR& GetUpPos() const { return Up; }
	const XMFLOAT3& GetXYZ() const { return camXYZPos; }
	const UINT& GetBufferWidth() const { return width; }
	const UINT& GetBufferHeight() const { return height; }

	//const XMMATRIX& GetView() const { return g_View; }
	//const XMMATRIX& GetProjection() const { return g_Projection; }

	// Setters
	//void SetEyePos(XMVECTOR newEyePos);
	//void SetAtPos(XMVECTOR newAtPos);
	//void SetUpPos(XMVECTOR newUpPos);

	void SetBufferWidthPos(UINT newWidth);
	void SetBufferHeight(UINT newHeight);

private:
	bool camPosMoved = false;

	XMFLOAT3 camPos = { 0.0f, 1.0f, -5.0f };

	XMFLOAT3 camXYZPos = { 0.0f, 1.0f, -5.0f };


	UINT width;
	UINT height;
};

