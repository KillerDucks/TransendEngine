// GUtility.h
#pragma once
#include <d3d11_1.h>
#include <directxmath.h>
#include <SpriteFont.h>
#include <SimpleMath.h>

using namespace DirectX;
inline XMVECTOR GMathFV(XMFLOAT3& val)
{
	return XMLoadFloat3(&val);
}
inline XMFLOAT3 GMathVF(XMVECTOR& vec)
{
	XMFLOAT3 val;
	XMStoreFloat3(&val, vec);
	return val;
}