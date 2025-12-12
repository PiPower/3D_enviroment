#include "Body.hpp"

using namespace DirectX;

void Body::UpdateBody(
	float dt)
{
	XMVECTOR newPosition = XMLoadFloat3(&linVelocity) * dt + XMLoadFloat3(&position);
	XMStoreFloat3(&position, newPosition);
}

void Body::ApplyLinearImpulse(
	XMFLOAT3* Impulse)
{
	if (massInv == 0.0f)
	{
		return;
	}

	linVelocity.x += Impulse->x * massInv;
	linVelocity.y += Impulse->y * massInv;
	linVelocity.z += Impulse->z * massInv;
}

void Body::GetCenterOfMass(
	XMFLOAT3* centerOfMass)
{
	shape.getCenterOfMass(&shape, centerOfMass);
	XMVECTOR Q = XMLoadFloat4(&rotation);
	XMVECTOR Q_inv = XMQuaternionInverse(Q);
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR CoM = XMLoadFloat3(centerOfMass);

	XMStoreFloat3(centerOfMass, pos + XMQuaternionMultiply(XMQuaternionMultiply(Q, CoM), Q_inv));
}
