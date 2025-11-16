#pragma once
#include "Body.hpp"

struct Contact
{
	DirectX::XMFLOAT3 normal;
	Body* bodyA;
	Body* bodyB;
	float timeOfImpact;
};

bool CheckIntersection(Body* bodyA, Body* bodyB, Contact* contact, float dt);
