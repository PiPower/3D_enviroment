#pragma once
#include <vector>
#include "Body.hpp"
#include "Intersection.hpp"


struct BodyPlaneDistance
{
	size_t bodyId;
	float distance;
	bool isMin;
};

struct PhysicsEnigne
{
	PhysicsEnigne(size_t expectedDynamicBodies = 200, size_t expectedStaticBodies = 200);

	int64_t AddBody(
		const BodyProperties& props,
		ShapeType shapeType,
		DirectX::XMFLOAT3 scales,
		bool isDynamic, 
		uint64_t* bodyId,
		DirectX::XMFLOAT3 constForce = {0, -9.8, 0});
	
	int64_t GetTransformMatrixForBody(
		uint64_t bodyId,
		DirectX::XMFLOAT4X4* mat);

	int64_t UpdateBodies(float dt);

	int64_t FindIntersections(
		float dt);

	Body* GetBody(
		uint64_t bodyId);

	Shape CreateDefaultShape(
		ShapeType type,
		DirectX::XMFLOAT3 scales);

	void ResolveContact(
		Contact* contact
	);

	void SortBodiesByDistanceToPlane(
		 const DirectX::XMFLOAT3* normal,
		float dt);

	void CreateCollisionPairs();

	void BroadPhase(float dt);

	void GetAngularImpulse(
		const Body* body, 
		const DirectX::XMFLOAT3* point,
		const DirectX::XMFLOAT3* normal, 
		DirectX::XMFLOAT3* Impulse);

public:
	std::vector<Body> staticBodies;
	std::vector<DirectX::XMFLOAT3> constForces; // per dynamic body
	std::vector<Body> dynamicBodies;
	std::vector<Contact> contactPoints;
	std::vector<BodyPlaneDistance> sortedBodies;
	size_t detectedIntersections;
};

