#pragma once
#include "Body.hpp"

struct Contact
{
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 ptOnA;
	DirectX::XMFLOAT3 ptOnB;
	float timeOfImpact;
	Body* bodyA;
	Body* bodyB;
};

bool CheckIntersection(
	Body* bodyA,
	Body* bodyB,
	Contact* contact,
	float dt);
