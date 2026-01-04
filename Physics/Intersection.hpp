#pragma once
#include "Body.hpp"

struct Contact
{
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 ptOnA;
	DirectX::XMFLOAT3 ptOnB;
	DirectX::XMFLOAT3 localPtOnA;
	DirectX::XMFLOAT3 localPtOnB;
	float timeOfImpact;
	Body* bodyA;
	Body* bodyB;
};

bool CheckIntersection(
	Body* bodyA,
	Body* bodyB,
	Contact* contact,
	float dt);

void DistanceBetweenBodies(
	Body* bodyA,
	Body* bodyB,
	DirectX::XMFLOAT3* ptOnA,
	DirectX::XMFLOAT3* ptOnB,
	float* dist);