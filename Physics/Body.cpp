#include "Body.hpp"

using namespace DirectX;

void UpdateBody(Body* body, float dt)
{
	XMVECTOR position = XMLoadFloat3(&body->linVelocity) * dt + XMLoadFloat3(&body->position);
	XMStoreFloat3(&body->position, position);
}