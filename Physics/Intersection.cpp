#include "Intersection.hpp"
#include <cstdlib>

using namespace DirectX;

static bool inline CompareSigns(
	float a,
	float b)
{
	return (a > 0 && b > 0) || (b < 0 && a < 0);
}

static void SignedDistance1D(
	XMFLOAT3* simplex, 
	float* lambdas)
{
	XMVECTOR s1 = XMLoadFloat3(&simplex[0]);
	XMVECTOR s2 = XMLoadFloat3(&simplex[1]);
	XMVECTOR o = XMVectorZero();
	XMVECTOR v = s2 - s1;

	XMVECTOR p0 = s1 + v * XMVector3Dot(o - s1, v) / XMVector3LengthSq(v);

	float mu[3];
	XMStoreFloat3((XMFLOAT3*) &mu, s1 - s2);
	float mu_max = mu[0];
	int mu_idx = 0;
	for (int i = 1; i < 3; i++)
	{
		if (mu_max * mu_max < mu[i] * mu[i])
		{
			mu_max = mu[i];
			mu_idx = i;
		}
	}
	float C1 = XMVectorGetByIndex(p0 - s2, mu_idx);
	float C2 = XMVectorGetByIndex(s1 - p0, mu_idx);

	// if p is between [s1, s2]
	if (CompareSigns(mu_max, C1) && CompareSigns(mu_max, C2)) {
		lambdas[0] = C1 / mu_max;
		lambdas[1] = C2 / mu_max;
	}
	else
	{
		lambdas[0] = 1.0f;
		lambdas[1] = 0;
	}
	return;
}

static void SignedDistance2D(
	XMFLOAT3* simplex,
	float* lambdas)
{
	XMVECTOR s1 = XMLoadFloat3(&simplex[0]);
	XMVECTOR s2 = XMLoadFloat3(&simplex[1]);
	XMVECTOR s3 = XMLoadFloat3(&simplex[2]);

	XMVECTOR n = XMVector3Cross(s2 - s1, s3 - s1);
	XMVECTOR p0 = (XMVector3Dot(s1, n) * n) / XMVector3LengthSq(n);

	float diff21[3], diff31[3];
	XMStoreFloat3((XMFLOAT3*)&diff21, s2 - s1);
	XMStoreFloat3((XMFLOAT3*)&diff31, s3 - s1);

	uint8_t idx = 0;
	float mu_max = 0;
	for (uint8_t i = 0; i < 3; i++)
	{
		uint8_t k = (i + 1) % 3;
		uint8_t l = (i + 2) % 3;

		float mu = diff21[k] * diff31[l] - diff21[l] * diff31[k];
		if (mu * mu > mu_max * mu_max)
		{
			mu_max = mu;
			idx = i;
		}
	}

	uint8_t x = (idx + 1) % 3;
	uint8_t y = (idx + 2) % 3;
	float diffSP[9];
	XMStoreFloat3((XMFLOAT3*)diffSP, s1 - p0);
	XMStoreFloat3((XMFLOAT3*)(diffSP + 3), s2 - p0);
	XMStoreFloat3((XMFLOAT3*)(diffSP + 6), s3 - p0);

	float C[3];
	for (uint8_t i = 0; i < 3; i++)
	{
		uint8_t k = (i + 1) % 3;
		uint8_t l = (i + 2) % 3;

		C[i] = diffSP[k * 3 + x] * diffSP[l * 3 + y] - diffSP[k * 3 + y] * diffSP[l * 3 + x];
	}

	if (CompareSigns(mu_max, C[0]) && CompareSigns(mu_max, C[1]) && CompareSigns(mu_max, C[2]))
	{
		lambdas[0] = C[0] / mu_max;
		lambdas[1] = C[1] / mu_max;
		lambdas[2] = C[2] / mu_max;
		return;
	}

	float dist = 1000000000.0f;
	for (uint8_t i = 0; i < 3; i++)
	{
		if (!CompareSigns(mu_max, -C[i]))
		{
			continue;
		}

		uint8_t k = (i + 1) % 3;
		uint8_t l = (i + 2) % 3;
		XMFLOAT3 simplexEdge[2];
		float edgeLambdas[2];
		simplexEdge[0] = simplex[k];
		simplexEdge[1] = simplex[l];

		SignedDistance1D(simplexEdge, edgeLambdas);
		XMVECTOR v = XMVectorZero();

		v += edgeLambdas[0] * XMLoadFloat3(&simplexEdge[0]);
		v += edgeLambdas[1] * XMLoadFloat3(&simplexEdge[1]);

		float distSq;
		XMStoreFloat(&distSq, XMVector3LengthSq(v));
		if (distSq < dist)
		{
			lambdas[0] = edgeLambdas[0];
			lambdas[1] = edgeLambdas[1];
			lambdas[2] = 0.0f;

			simplex[0] = simplexEdge[0];
			simplex[1] = simplexEdge[1];
		}
	}
}

static void SignedDistance3D(
	XMFLOAT3* simplex,
	float* lambdas)
{

}
/*
	Base od distance algorithm sets 
		idxCount to number of valid points in simplex
		lambdas to correct values of lambda 
		newDir to new direction  
*/
static void DistanceSubalgorithm(
	XMFLOAT3* simplex,
	uint8_t* idxCount,
	float* lambdas,
	XMFLOAT3* newDir)
{
	XMVECTOR v = XMVectorZero();

	switch (*idxCount)
	{
	case 2:
		SignedDistance1D(simplex, lambdas);
		v += lambdas[0] * XMLoadFloat3(&simplex[0]);
		v += lambdas[1] * XMLoadFloat3(&simplex[1]);
		v = v * -1.0f;

		XMStoreFloat3(newDir, v);
		*idxCount = lambdas[1] == 0.0f ? 1 : 2;
		break;
	case 3:
		SignedDistance2D(simplex, lambdas);
		v += lambdas[0] * XMLoadFloat3(&simplex[0]);
		v += lambdas[1] * XMLoadFloat3(&simplex[1]);
		v += lambdas[2] * XMLoadFloat3(&simplex[2]);
		v = v * -1.0f;

		XMStoreFloat3(newDir, v);
		*idxCount = lambdas[2] == 0.0f ? 2 : 3;
		*idxCount = (*idxCount == 2 && lambdas[1] == 0.0f) ? 1 : 2;
		break;
	case 4: 
		SignedDistance3D(simplex, lambdas);
		break;
	default:
		exit(-1);
	}
}

static void GetSupport(
	Body* bodyA, 
	Body* bodyB, 
	const XMFLOAT3* dir, 
	XMFLOAT3* support,
	float bias)
{
	XMFLOAT3 supportA = {}, supportB = {};
	XMFLOAT3 negateDir = { dir->x * -1.0f, dir->y * -1.0f, dir->z * -1.0f };

	bodyA->shape.supportFunction(&bodyA->shape, &bodyA->position, dir, &supportA, bias);
	bodyB->shape.supportFunction(&bodyB->shape, &bodyB->position, &negateDir, &supportB, bias);

	XMVECTOR VecSupportA = XMLoadFloat3(&supportA);
	XMVECTOR VecSupportB = XMLoadFloat3(&supportB);

	XMStoreFloat3(support, XMVectorSubtract(VecSupportA, VecSupportB));
}

static bool HasPoint(
	const XMFLOAT3* simplex,
	const XMFLOAT3* newPoint,
	const uint8_t idxCount)
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

static bool GjkIntersectionTest(
	Body* bodyA,
	Body* bodyB,
	Contact* contact,
	float bias)
{
	XMFLOAT3 simplex[4];
	uint8_t idxCount = 1;
	XMFLOAT3 dir = { 0, 1, 0 };
	float lambdas[4] = {};
	XMFLOAT3 support;
	constexpr float epsilon = 0.001;

	GetSupport(bodyA, bodyB, &dir, &simplex[0], bias);
	XMVECTOR vecDir = XMLoadFloat3(&simplex[0]) * -1.0f;
	XMStoreFloat3(&dir, vecDir);
	bool hasOrigin = false;
	float closestDistSq = simplex[0].x * simplex[0].x + simplex[0].y * simplex[0].y + simplex[0].z * simplex[0].z;
	while (!hasOrigin)
	{
		vecDir = XMLoadFloat3(&dir);
		GetSupport(bodyA, bodyB, &dir, &support, bias);
		if (HasPoint(simplex, &support, idxCount))
		{
			break;
		}
		simplex[idxCount] = support;
		idxCount++;

		float dotProd;
		XMStoreFloat(&dotProd, XMVector3Dot(vecDir, XMLoadFloat3(&simplex[idxCount- 1])));
		if (dotProd < 0.0f)
		{
			break;
		}

		DistanceSubalgorithm(simplex, &idxCount, lambdas, &dir);
		float newDistSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

		if (newDistSq < epsilon * epsilon)
		{
			hasOrigin = true;
			break;
		}

		if (newDistSq > closestDistSq)
		{
			break;
		}

		closestDistSq = newDistSq;
		hasOrigin = idxCount == 4;
	}

	return hasOrigin;
}

bool CheckIntersection(
	Body* bodyA,
	Body* bodyB,
	Contact* contact,
	float dt)
{
	// divide time into sections and iterate
	constexpr uint8_t ITERS = 10;
	float stepSize = dt / (float)ITERS;
	float total_time = 0;
	for (size_t i = 0; i < ITERS; i++)
	{
		if (GjkIntersectionTest(bodyA, bodyB, contact, 0.001))
		{
			contact->timeOfImpact = total_time;
			return true;
		}

		UpdateBody(bodyA, stepSize);
		UpdateBody(bodyB, stepSize);
		total_time += stepSize;
	}

	UpdateBody(bodyA, -dt);
	UpdateBody(bodyB, -dt);
	return false;
}
