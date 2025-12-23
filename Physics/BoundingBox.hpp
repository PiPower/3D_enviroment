#pragma once
#include <DirectXMath.h>

struct BoundingBox
{
	BoundingBox() : minC({1e12, 1e12, 1e12}), maxC({-1e12, -1e12, -1e12}) {};

	BoundingBox(
		const DirectX::XMFLOAT3* pt,
		size_t num);

	void Expand(
		const DirectX::XMFLOAT3* pt,
		size_t num);

	void Expand(
		const DirectX::XMFLOAT3& pt);

	DirectX::XMFLOAT3 minC;
	DirectX::XMFLOAT3 maxC;
}; 
