#include "Intersection.hpp"

using namespace DirectX;

static void DistanceSubalgorithm(XMFLOAT3* simplex, uint8_t idxCount, XMFLOAT4* lambdas, XMFLOAT3* newDir)
{
	switch (idxCount)
	{
	default:
		break;
	}
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

static bool HasPoint(const XMFLOAT3* simplex, const XMFLOAT3* newPoint, const uint8_t idxCount)
{
	constexpr float eps = 0.0001;
	XMVECTOR vecNewPoint = XMLoadFloat3(newPoint);
	for (uint8_t i = 0; i < idxCount; i++)
	{
		XMVECTOR diffVec = XMLoadFloat3(&simplex[i]) - vecNewPoint;
		float lengthSq;
		XMStoreFloat(&lengthSq, XMVector3LengthSq(diffVec));
		if (lengthSq < eps * eps)
		{
			return true;
		}
	}
	return false;
}

static bool GjkIntersectionTest(Body* bodyA, Body* bodyB, Contact* contact, float bias)
{
	XMFLOAT3 simplex[4];
	uint8_t idxCount = 1;
	XMFLOAT3 dir = { 1, 0, 0 };
	XMFLOAT4 lambdas = {};
	XMFLOAT3 support;

	GetSupport(bodyA, bodyB, &dir, &simplex[0], bias);
	XMVECTOR vecDir = XMLoadFloat3(&simplex[0]);
	XMStoreFloat3(&dir,vecDir * -1.0f);
	bool hasOrigin = false;
	float closestDist = sqrtf(simplex[0].x * simplex[0].x + simplex[0].y * simplex[0].y + simplex[0].z * simplex[0].z);
	while (!hasOrigin)
	{
		GetSupport(bodyA, bodyB, &dir, &support, bias);
		if (HasPoint(simplex, &support, idxCount))
		{
			break;
		}
		simplex[idxCount] = support;
		idxCount++;

		float dotProd;
		XMStoreFloat(&dotProd, XMVector3Dot(vecDir, XMLoadFloat3(&simplex[idxCount])));
		if (dotProd)
		{
			break;
		}

		DistanceSubalgorithm(simplex, idxCount, &lambdas, &dir);
		
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
