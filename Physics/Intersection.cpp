#include "Intersection.hpp"
#include <cstdlib>
#include <vector>
using namespace std;
using namespace DirectX;


template<typename T>
bool inline CheckIfNotNaN(const T& v)
{
	return v * 0 == v * 0; 
}

struct Triangle
{
	uint16_t a; 
	uint16_t b;
	uint16_t c;
};

struct Edge
{
	uint16_t i;
	uint16_t j;


	bool operator == (const Edge& rhs) const {
		return ((i== rhs.i && j == rhs.j) || (i == rhs.j && j == rhs.i));
	}
};

struct SupportPoint
{
	XMFLOAT3 ptOnSimplex;
	XMFLOAT3 ptOnA;
	XMFLOAT3 ptOnB;
};

struct Simplex
{
	XMFLOAT3 ptOnSimplex[4];
	uint8_t idxCount;
	XMFLOAT3 ptOnA[4];
	XMFLOAT3 ptOnB[4];

	Simplex() : idxCount(0), ptOnSimplex(), ptOnA(), ptOnB() {}

	void AddSupport(const SupportPoint* supp)
	{
		ptOnSimplex[idxCount] = supp->ptOnSimplex;
		ptOnA[idxCount] = supp->ptOnA;
		ptOnB[idxCount] = supp->ptOnB;
		idxCount++;
	}
	void AddSupport(const Simplex* simplex, const uint8_t idx)
	{
		ptOnSimplex[idxCount] = simplex->ptOnSimplex[idx];
		ptOnA[idxCount] = simplex->ptOnA[idx];
		ptOnB[idxCount] = simplex->ptOnB[idx];
		idxCount++;
	}
};

static bool inline CompareSigns(
	float a,
	float b)
{
	return (a > 0 && b > 0) || (b < 0 && a < 0);
}

static void GetSupport(
	const Body* bodyA,
	const Body* bodyB,
	const XMFLOAT3* dir,
	XMFLOAT3* support,
	XMFLOAT3* ptOnA,
	XMFLOAT3* ptOnB,
	float bias)
{
	XMFLOAT3 negateDir = { dir->x * -1.0f, dir->y * -1.0f, dir->z * -1.0f };

	bodyA->shape.supportFunction(&bodyA->shape, &bodyA->position, dir, ptOnA, bias);
	bodyB->shape.supportFunction(&bodyB->shape, &bodyB->position, &negateDir, ptOnB, bias);

	XMVECTOR VecSupportA = XMLoadFloat3(ptOnA);
	XMVECTOR VecSupportB = XMLoadFloat3(ptOnB);

	XMStoreFloat3(support, XMVectorSubtract(VecSupportA, VecSupportB));
}


static void GetSupport(
	const Body* bodyB,
	const Body* bodyA,
	const XMFLOAT3* dir,
	SupportPoint* supportPoint,
	float bias)
{
	return GetSupport(bodyA, bodyB, dir, &supportPoint->ptOnSimplex, &supportPoint->ptOnA, &supportPoint->ptOnB, bias);
}

static float SignedDistanceToSurface(
	const XMFLOAT3* p0,
	const XMFLOAT3* p1,
	const XMFLOAT3* p2,
	const XMFLOAT3* point)
{
	XMVECTOR vec10 = XMLoadFloat3(p1) - XMLoadFloat3(p0);
	XMVECTOR vec20 = XMLoadFloat3(p2) - XMLoadFloat3(p0);
	XMVECTOR vecP0 = XMLoadFloat3(point) - XMLoadFloat3(p0);
	XMVECTOR n = XMVector3Cross(vec10, vec20);
	n = XMVector3Normalize(n);

	float dist;
	XMStoreFloat(&dist, XMVector3Dot(n, vecP0));
	return dist;
}

static size_t RemoveTrianglesFacingPoint(
	std::vector<Triangle>* triangles,
	const std::vector<SupportPoint>& vertecies,
	const SupportPoint& pt)
{
	size_t removedTriangles = 0;
	size_t triangleCount = triangles->size();
	for (size_t i = 0; i < triangleCount; i++)
	{
		const Triangle& tri = (*triangles)[i];
		float dist = SignedDistanceToSurface(&vertecies[tri.a].ptOnSimplex,
			&vertecies[tri.b].ptOnSimplex, &vertecies[tri.c].ptOnSimplex, &pt.ptOnSimplex);

		if (dist > 0.0f)
		{
			// This triangle faces the point.  Remove it.
			triangles->erase(triangles->begin() + i);
			i--;
			triangleCount--;
			removedTriangles++;
		}
	}
	return removedTriangles;
}

static bool HasPoint(
	const SupportPoint& point,
	const std::vector<Triangle>& triangles, 
	const std::vector<SupportPoint>& vertecies) 
{
	const float epsilons = 0.001f * 0.001f;
	XMVECTOR vec = XMLoadFloat3(&point.ptOnSimplex);
	XMVECTOR delta;

	for (int i = 0; i < triangles.size(); i++) 
	{
		const Triangle& tri = triangles[i];
		XMVECTOR a = XMLoadFloat3(&vertecies[tri.a].ptOnSimplex);
		XMVECTOR b = XMLoadFloat3(&vertecies[tri.b].ptOnSimplex);
		XMVECTOR c = XMLoadFloat3(&vertecies[tri.c].ptOnSimplex);

		float distA;
		XMStoreFloat(&distA, XMVector3LengthSq(vec - a));
		if (distA < epsilons)
		{
			return true;
		}

		float distB;
		XMStoreFloat(&distB, XMVector3LengthSq(vec - b));
		if (distB < epsilons)
		{
			return true;
		}

		float distC;
		XMStoreFloat(&distC, XMVector3LengthSq(vec - c));
		if (distC < epsilons )
		{
			return true;
		}

	}
	return false;
}


static size_t NearestTriangleToPoint(
	const std::vector<Triangle>& triangles, 
	const std::vector<SupportPoint>& vertecies,
	const XMFLOAT3* point)
{
	float distSq = 1e10;
	size_t idx = 0;

	for (size_t i = 0; i < triangles.size(); i++)
	{
		const Triangle& tri = triangles[i];
		float localDist = SignedDistanceToSurface(&vertecies[tri.a].ptOnSimplex,
					&vertecies[tri.b].ptOnSimplex, &vertecies[tri.c].ptOnSimplex, point);
		// localDist is negative so square is needed to remove - sign
		if (localDist * localDist < distSq)
		{
			idx = i;
			distSq = localDist * localDist;
		}
	}

	return idx;
}

static void FindDanglingEdges(
	vector<Edge>* danglingEdges,
	const vector<Triangle>& triangles)
{
	for (int i = 0; i < triangles.size(); i++) 
	{
		const Triangle& tri = triangles[i];

		Edge edges[3];
		edges[0].i = tri.a;
		edges[0].j = tri.b;

		edges[1].i = tri.b;
		edges[1].j = tri.c;

		edges[2].i = tri.c;
		edges[2].j = tri.a;

		int counts[3];
		counts[0] = 0;
		counts[1] = 0;
		counts[2] = 0;

		for (int j = 0; j < triangles.size(); j++) {
			if (j == i) {
				continue;
			}

			const Triangle& tri2 = triangles[j];

			Edge edges2[3];
			edges2[0].i = tri2.a;
			edges2[0].j = tri2.b;

			edges2[1].i = tri2.b;
			edges2[1].j = tri2.c;

			edges2[2].i = tri2.c;
			edges2[2].j = tri2.a;

			for (int k = 0; k < 3; k++) 
			{
				if (edges[k] == edges2[0])
				{
					counts[k]++;
				}
				if (edges[k] == edges2[1]) 
				{
					counts[k]++;
				}
				if (edges[k] == edges2[2]) 
				{
					counts[k]++;
				}
			}
		}

		for (int k = 0; k < 3; k++) 
		{
			if (0 == counts[k])
			{
				danglingEdges->push_back(edges[k]);
			}
		}
	}
}

static void BarycentricCoords(
	XMFLOAT3* p1,
	XMFLOAT3* p2,
	XMFLOAT3* p3,
	float* lambdas)
{
	XMVECTOR s1 = XMLoadFloat3(p1);
	XMVECTOR s2 = XMLoadFloat3(p2);
	XMVECTOR s3 = XMLoadFloat3(p3);

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

	// check if every component is not NaN 
	if (CheckIfNotNaN(C[0]) || CheckIfNotNaN(C[1]) || CheckIfNotNaN(C[2]))
	{
		lambdas[0] = C[0] / mu_max;
		lambdas[1] = C[1] / mu_max;
		lambdas[2] = C[2] / mu_max;
		return;
	}
	else
	{
		lambdas[0] = 1.0f;
		lambdas[1] = 0;
		lambdas[2] = 0;
	}

}

static float EpaContactInfo(
	const Body* bodyA, 
	const Body* bodyB,
	const float bias, 
	const Simplex simplexPoints[4],
	XMFLOAT3* ptOnA,
	XMFLOAT3* ptOnB)
{

	vector<SupportPoint> points;
	vector<Triangle> triangles;
	vector<Edge> danglingEdges;

	XMFLOAT3 center = {};
	for (int i = 0; i < 4; i++)
	{
		SupportPoint simplexPoint = { simplexPoints->ptOnSimplex[i], simplexPoints->ptOnA[i] , simplexPoints->ptOnB[i] };
		points.push_back(simplexPoint);
		center.x += simplexPoint.ptOnSimplex.x;
		center.y += simplexPoint.ptOnSimplex.y;
		center.z += simplexPoint.ptOnSimplex.z;
	}

	center.x *= 0.25; 
	center.y *= 0.25; 
	center.z *= 0.25; 
	// Build the triangles
	for (int i = 0; i < 4; i++) 
	{
		int j = (i + 1) % 4;
		int k = (i + 2) % 4;
		Triangle tri;
		tri.a = i;
		tri.b = j;
		tri.c = k;
		uint16_t unusedPt = (i + 3) % 4;

		// check if normal is oriented outward
		float dist = SignedDistanceToSurface(&points[tri.a].ptOnSimplex,
			&points[tri.b].ptOnSimplex, &points[tri.c].ptOnSimplex, &points[unusedPt].ptOnSimplex);
		if (dist > 0.0f) 
		{
			std::swap(tri.a, tri.b);
		}
		triangles.push_back(tri);
	}
	XMFLOAT3 origin = { 0, 0, 0 };
	XMFLOAT3 normal;
	SupportPoint suppPoint;

	while (true)
	{
		const size_t idx = NearestTriangleToPoint(triangles, points, &origin);
		const Triangle& tri = triangles[idx];
		XMVECTOR vecBA = XMLoadFloat3(&points[tri.b].ptOnSimplex) - XMLoadFloat3(&points[tri.a].ptOnSimplex);
		XMVECTOR vecCA = XMLoadFloat3(&points[tri.c].ptOnSimplex) - XMLoadFloat3(&points[tri.a].ptOnSimplex);
		XMVECTOR n = XMVector3Cross(vecBA, vecCA);
		XMStoreFloat3(&normal, XMVector3Normalize(n));
		GetSupport(bodyA, bodyB, &normal, &suppPoint, bias);

		if (HasPoint(suppPoint, triangles, points))
		{
			break;
		}
		float dist = SignedDistanceToSurface(&points[tri.a].ptOnSimplex,
				&points[tri.b].ptOnSimplex, &points[tri.c].ptOnSimplex, &suppPoint.ptOnSimplex);
		if (dist <= 0.0f) 
		{
			break;	// can't expand
		}

		points.push_back(suppPoint);
		size_t numRemoved = RemoveTrianglesFacingPoint(&triangles, points, suppPoint);
		if (0 == numRemoved) 
		{
			break;
		}

		danglingEdges.clear();
		FindDanglingEdges(&danglingEdges, triangles);

		if (0 == danglingEdges.size())
		{
			break;
		}

		for (int i = 0; i < danglingEdges.size(); i++) 
		{
			const Edge& edge = danglingEdges[i];

			Triangle tri;
			tri.a = points.size() - 1;
			tri.b = edge.j;
			tri.c = edge.i;

			// Make sure it's oriented properly
			float dist = SignedDistanceToSurface(&points[tri.a].ptOnSimplex,
				&points[tri.b].ptOnSimplex, &points[tri.c].ptOnSimplex, &center);

			if (dist > 0.0f) 
			{
				std::swap(tri.b, tri.c);
			}

			triangles.push_back(tri);
		}

	}
	const int idx = NearestTriangleToPoint(triangles, points, &origin);
	const Triangle& tri = triangles[idx];
	float lambdas[3];
	BarycentricCoords(&points[tri.a].ptOnSimplex, &points[tri.b].ptOnSimplex, &points[tri.c].ptOnSimplex, lambdas);

	XMVECTOR vecPtA = XMLoadFloat3(&points[tri.a].ptOnA) * lambdas[0] +
		XMLoadFloat3(&points[tri.b].ptOnA) * lambdas[1] +
		XMLoadFloat3(&points[tri.c].ptOnA) * lambdas[2];


	XMVECTOR vecPtB = XMLoadFloat3(&points[tri.a].ptOnB) * lambdas[0] +
		XMLoadFloat3(&points[tri.b].ptOnB) * lambdas[1] +
		XMLoadFloat3(&points[tri.c].ptOnB) * lambdas[2];
	
	XMStoreFloat3(ptOnA, vecPtA);
	XMStoreFloat3(ptOnB, vecPtB);

	float dist;
	XMStoreFloat(&dist, XMVector3Length(vecPtA - vecPtB));
	return dist;
}

static inline void GetOrtho(
	XMFLOAT3* n, 
	XMFLOAT3* u, 
	XMFLOAT3* v)
{
	XMVECTOR nVec = XMLoadFloat3(n);
	nVec = XMVector3Normalize(nVec);

	XMVECTOR uVec, vVec;
	XMFLOAT3 imm;
	XMStoreFloat3(&imm, nVec * nVec);
	XMVECTOR wVec = (imm.z > 0.9f * 0.9f) ? XMVectorSet(1, 0, 0 ,0) : XMVectorSet(0, 1, 0, 0);
	uVec = XMVector3Cross(wVec, nVec);
	uVec = XMVector3Normalize(uVec);


	vVec = XMVector3Cross(nVec, uVec);
	vVec = XMVector3Normalize(vVec);
	uVec = XMVector3Cross(vVec, nVec);
	uVec = XMVector3Normalize(uVec);

	XMStoreFloat3(u, uVec);
	XMStoreFloat3(v, vVec);
}


static void SignedDistance1D(
	Simplex* simplex, 
	float* lambdas)
{
	XMVECTOR s1 = XMLoadFloat3(&simplex->ptOnSimplex[0]);
	XMVECTOR s2 = XMLoadFloat3(&simplex->ptOnSimplex[1]);
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
	Simplex* simplex,
	float* lambdas)
{

	XMVECTOR s1 = XMLoadFloat3(&simplex->ptOnSimplex[0]);
	XMVECTOR s2 = XMLoadFloat3(&simplex->ptOnSimplex[1]);
	XMVECTOR s3 = XMLoadFloat3(&simplex->ptOnSimplex[2]);

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
	Simplex simplexBuffer;
	for (uint8_t i = 0; i < 3; i++)
	{

		uint8_t k = (i + 1) % 3;
		uint8_t l = (i + 2) % 3;
		Simplex simplexLocal;
		float edgeLambdas[2];
		simplexLocal.AddSupport(simplex, k);
		simplexLocal.AddSupport(simplex, l);

		SignedDistance1D(&simplexLocal, edgeLambdas);
		XMVECTOR v = XMVectorZero();

		v += edgeLambdas[0] * XMLoadFloat3(&simplexLocal.ptOnSimplex[0]);
		v += edgeLambdas[1] * XMLoadFloat3(&simplexLocal.ptOnSimplex[1]);

		float distSq;
		XMStoreFloat(&distSq, XMVector3LengthSq(v));
		if (distSq < dist)
		{
			dist = distSq;

			lambdas[0] = edgeLambdas[0];
			lambdas[1] = edgeLambdas[1];
			lambdas[2] = 0.0f;

			simplexBuffer = simplexLocal;
		}
	}
	*simplex = simplexBuffer;
}

static void SignedDistance3D(
	Simplex* simplex,
	float* lambdas)
{
	XMVECTOR column4 = XMVectorSet(0, 0, 0, 1.0f);
	float C[4];
	float determinant = 0;
	int minorColumns[4][3] = { {1, 2, 3}, {0, 2, 3}, {0 , 1, 3}, {0, 1, 2} };
	for (uint8_t i = 0; i < 4; i++)
	{
		XMVECTOR column1 = XMLoadFloat3(&simplex->ptOnSimplex[minorColumns[i][0]]);
		XMVECTOR column2 = XMLoadFloat3(&simplex->ptOnSimplex[minorColumns[i][1]]);
		XMVECTOR column3 = XMLoadFloat3(&simplex->ptOnSimplex[minorColumns[i][2]]);

		XMMATRIX mat = XMMatrixTranspose( XMMATRIX(column1, column2, column3, column4) );
		XMStoreFloat(&C[i], XMMatrixDeterminant(mat));
		C[i] *= (4 + i) % 2 == 0 ? 1 : -1;
		determinant += C[i];
	}

	if (CompareSigns(determinant, C[0]) && CompareSigns(determinant, C[1])
		&& CompareSigns(determinant, C[2]) && CompareSigns(determinant, C[3]))
	{
		lambdas[0] = C[0] / determinant;
		lambdas[1] = C[1] / determinant;
		lambdas[2] = C[2] / determinant;
		lambdas[3] = C[3] / determinant;
		return;
	}


	float dist = 1000000000.0f;
	Simplex simplexBuffer;
	for (uint8_t i = 0; i < 4; i++)
	{
		uint8_t k = (i + 1) % 4;
		uint8_t l = (i + 2) % 4;
		Simplex localSimplex;
		float edgeLambdas[3];
		localSimplex.AddSupport(simplex, i);
		localSimplex.AddSupport(simplex, k);
		localSimplex.AddSupport(simplex, l);

		SignedDistance2D(&localSimplex, edgeLambdas);
		XMVECTOR v = XMVectorZero();

		v += edgeLambdas[0] * XMLoadFloat3(&localSimplex.ptOnSimplex[0]);
		v += edgeLambdas[1] * XMLoadFloat3(&localSimplex.ptOnSimplex[1]);
		v += edgeLambdas[2] * XMLoadFloat3(&localSimplex.ptOnSimplex[2]);

		float distSq;
		XMStoreFloat(&distSq, XMVector3LengthSq(v));
		if (distSq < dist)
		{
			dist = distSq;

			lambdas[0] = edgeLambdas[0];
			lambdas[1] = edgeLambdas[1];
			lambdas[2] = edgeLambdas[2];
			lambdas[3] = 0.0f;

			simplexBuffer = localSimplex;
		}
	}
	*simplex = simplexBuffer;

}
/*
	Base od distance algorithm sets 
		idxCount to number of valid points in simplex
		lambdas to correct values of lambda 
		newDir to new direction  
*/
static void DistanceSubalgorithm(
	Simplex* simplex,
	float* lambdas,
	XMFLOAT3* newDir)
{
	XMVECTOR v = XMVectorZero();

	switch (simplex->idxCount)
	{
	case 2:
		SignedDistance1D(simplex, lambdas);
		v += lambdas[0] * XMLoadFloat3(&simplex->ptOnSimplex[0]);
		v += lambdas[1] * XMLoadFloat3(&simplex->ptOnSimplex[1]);
		v = v * -1.0f;

		XMStoreFloat3(newDir, v);
		simplex->idxCount = lambdas[1] == 0.0f ? 1 : 2;
		break;
	case 3:
		SignedDistance2D(simplex, lambdas);
		v += lambdas[0] * XMLoadFloat3(&simplex->ptOnSimplex[0]);
		v += lambdas[1] * XMLoadFloat3(&simplex->ptOnSimplex[1]);
		v += lambdas[2] * XMLoadFloat3(&simplex->ptOnSimplex[2]);
		v = v * -1.0f;

		XMStoreFloat3(newDir, v);
		simplex->idxCount = lambdas[2] == 0.0f ? 2 : 3;
		if (simplex->idxCount == 2) { simplex->idxCount = lambdas[1] == 0.0f ? 1 : 2; }
		break;
	case 4: 
		SignedDistance3D(simplex, lambdas);
		v += lambdas[0] * XMLoadFloat3(&simplex->ptOnSimplex[0]);
		v += lambdas[1] * XMLoadFloat3(&simplex->ptOnSimplex[1]);
		v += lambdas[2] * XMLoadFloat3(&simplex->ptOnSimplex[2]);
		v += lambdas[3] * XMLoadFloat3(&simplex->ptOnSimplex[3]);
		v = v * -1.0f;

		XMStoreFloat3(newDir, v);
		simplex->idxCount = lambdas[3] == 0.0f ? 3 : 4;
		simplex->idxCount = lambdas[2] == 0.0f ? 2 : 3;
		simplex->idxCount = (simplex->idxCount == 2 && lambdas[1] == 0.0f) ? 1 : 2;
		break;
	default:
		exit(-1);
	}
}

static bool HasPoint(
	const Simplex* simplex,
	const SupportPoint* newPoint)
{
	constexpr float eps = 0.0001;
	XMVECTOR vecNewPoint = XMLoadFloat3(&newPoint->ptOnSimplex);
	for (uint8_t i = 0; i < simplex->idxCount; i++)
	{
		XMVECTOR diffVec = XMLoadFloat3(&simplex->ptOnSimplex[i]) - vecNewPoint;
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
	constexpr float epsilon = 0.0001;
	constexpr uint8_t maxIters = 10;

	Simplex simplex;
	XMFLOAT3 dir = { 0, 1, 0 };
	float lambdas[4] = {};
	SupportPoint support;
	uint8_t iterCount = 0;

	GetSupport(bodyA, bodyB, &dir, &support, bias);
	simplex.AddSupport(&support);
	XMVECTOR vecDir = XMLoadFloat3(&simplex.ptOnSimplex[0]) * -1.0f;
	XMStoreFloat3(&dir, vecDir);
	bool hasOrigin = false;
	float closestDistSq = simplex.ptOnSimplex[0].x * simplex.ptOnSimplex[0].x + 
						  simplex.ptOnSimplex[0].y * simplex.ptOnSimplex[0].y + 
						  simplex.ptOnSimplex[0].z * simplex.ptOnSimplex[0].z;
	while (!hasOrigin && iterCount < maxIters)
	{
		vecDir = XMLoadFloat3(&dir);
		GetSupport(bodyA, bodyB, &dir, &support, bias);
		if (HasPoint(&simplex, &support))
		{
			break;
		}
		simplex.AddSupport(&support);

		float dotProd;
		XMStoreFloat(&dotProd, XMVector3Dot(vecDir, XMLoadFloat3(&simplex.ptOnSimplex[simplex.idxCount- 1])));
		if (dotProd < 0.0f)
		{
			break;
		}

		DistanceSubalgorithm(&simplex, lambdas, &dir);
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
		hasOrigin = simplex.idxCount == 4;
		iterCount++;
	}

	if (!hasOrigin)
	{
		return false;
	}
	// if simplex is not Tetrahedron build it
	if (simplex.idxCount == 1)
	{
		XMFLOAT3 searchDir;
		XMStoreFloat3(&searchDir, XMLoadFloat3(&simplex.ptOnSimplex[0]) * -1.0f);

		SupportPoint supp;
		GetSupport(bodyA, bodyB, &searchDir, &supp, bias);
		simplex.AddSupport(&supp);
	}
	if (simplex.idxCount == 2)
	{
		XMFLOAT3 n = {simplex.ptOnSimplex[1].x - simplex.ptOnSimplex[0].x,
					  simplex.ptOnSimplex[1].y - simplex.ptOnSimplex[0].y,
					  simplex.ptOnSimplex[1].z - simplex.ptOnSimplex[0].z };
		XMFLOAT3 u, v;
		GetOrtho(&n, &u, &v);

		SupportPoint supp;
		GetSupport(bodyA, bodyB, &u, &supp, bias);
		simplex.AddSupport(&supp);
	}
	if (simplex.idxCount == 3)
	{
		XMVECTOR ab = XMLoadFloat3(&simplex.ptOnSimplex[1]) - XMLoadFloat3(&simplex.ptOnSimplex[0]);
		XMVECTOR ac = XMLoadFloat3(&simplex.ptOnSimplex[2]) - XMLoadFloat3(&simplex.ptOnSimplex[0]);
		XMFLOAT3 newDir;
		XMStoreFloat3(&newDir, XMVector3Cross(ab, ac));

		SupportPoint supp;
		GetSupport(bodyA, bodyB, &newDir, &supp, bias);
		simplex.AddSupport(&supp);
		
	}

	EpaContactInfo(bodyA, bodyB, bias, &simplex, &contact->ptOnA, &contact->ptOnB);
	return true;
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
			auto z = XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&contact->ptOnA);
			XMStoreFloat3(&contact->normal,
				XMVector3Normalize(XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&contact->ptOnA))
			);

			contact->timeOfImpact = total_time;
			contact->bodyA = bodyA;
			contact->bodyB = bodyB;
			bodyA->UpdateBody(-total_time);
			bodyB->UpdateBody(-total_time);
			return true;
		}
		bodyA->UpdateBody(stepSize);
		bodyB->UpdateBody(stepSize);
		total_time += stepSize;
	}

	bodyA->UpdateBody(-dt);
	bodyB->UpdateBody(-dt);

	return false;
}
