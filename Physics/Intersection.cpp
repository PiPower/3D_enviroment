#include "Intersection.hpp"

using namespace DirectX;

static void DistanceSubalgorithm()
{
	
}

static void GetSupport(Body* bodyA, Body* bodyB, const XMFLOAT3* dir, XMFLOAT3* support, float bias)
{
	XMFLOAT3 supportA = {}, supportB = {};
	XMFLOAT3 negateDir = { dir->x * -1.0f, dir->y * -1.0f, dir->z * -1.0f };

	bodyA->shape.supportFunction(&bodyA->shape, &bodyA->position, dir, &supportA, bias);
	bodyB->shape.supportFunction(&bodyB->shape, &bodyB->position, &negateDir, &supportB, bias);

	XMVECTOR VecSupportA = XMLoadFloat3(&supportA);
	XMVECTOR VecSupportB = XMLoadFloat3(&supportB);

	XMStoreFloat3(support, XMVectorSubtract(VecSupportA, VecSupportB));


}
static bool GjkIntersectionTest(Body* bodyA, Body* bodyB, Contact* contact, float bias)
{
	XMFLOAT3 simplex[4];
	uint8_t i = 1;
	XMFLOAT3 dir = { 1, 0, 0 };
	XMFLOAT3 support;

	GetSupport(bodyA, bodyB, &dir, &support, bias);
	simplex[0] = support;
	dir = { simplex[0].x * -1.0f,  simplex[0].y * -1.0f,  simplex[0].z * -1.0f };
	while (true)
	{
		GetSupport(bodyA, bodyB, &dir, &support, bias);

	}

	return false;
}

bool CheckIntersection(Body* bodyA, Body* bodyB, Contact* contact, float dt)
{
	// divide time into sections and iterate
	constexpr uint8_t ITERS = 10;
	float stepSize = dt / (float)ITERS;
	for (size_t i = 0; i < ITERS; i++)
	{
		if (GjkIntersectionTest(bodyA, bodyB, contact, 0.001))
		{
			return true;
		}

		UpdateBody(bodyA, stepSize);
		UpdateBody(bodyB, stepSize);
	}

	UpdateBody(bodyA, -dt);
	UpdateBody(bodyB, -dt);
	return false;
}
