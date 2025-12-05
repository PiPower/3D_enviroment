#include "Body.hpp"

using namespace DirectX;

void Body::UpdateBody(float dt)
{
	XMVECTOR newPosition = XMLoadFloat3(&linVelocity) * dt + XMLoadFloat3(&position);
	XMStoreFloat3(&position, newPosition);
}

void Body::ApplyLinearImpulse(XMFLOAT3* Impulse)
{
	if (massInv == 0.0f)
	{
		return;
	}

	linVelocity.x += Impulse->x * massInv;
	linVelocity.y += Impulse->y * massInv;
	linVelocity.z += Impulse->z * massInv;
}
