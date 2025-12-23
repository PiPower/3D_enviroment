#include "BoundingBox.hpp"

BoundingBox::BoundingBox(
	const DirectX::XMFLOAT3* pt,
	size_t num) :
	minC({1e12, 1e12, 1e12 }), maxC({-1e12 , -1e12 , -1e12 })
{
	Expand(pt, num);
}

void BoundingBox::Expand(
	const DirectX::XMFLOAT3* pt,
	size_t num)
{
	for (size_t i = 0; i < num; i++)
	{
		Expand(pt[i]);
	}
}

void BoundingBox::Expand(
	const DirectX::XMFLOAT3& pt)
{
	if (pt.x < minC.x) {
		minC.x = pt.x;
	}
	if (pt.y < minC.y) {
		minC.y = pt.y;
	}
	if (pt.z < minC.z) {
		minC.z = pt.z;
	}

	if (pt.x > maxC.x) {
		maxC.x = pt.x;
	}
	if (pt.y > maxC.y) {
		maxC.y = pt.y;
	}
	if (pt.z > maxC.z) {
		maxC.z = pt.z;
	}
}
