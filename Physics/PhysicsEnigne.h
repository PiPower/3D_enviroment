#pragma once
#include <vector>
#include "Body.hpp"
#include "Intersection.hpp"

struct PhysicsEnigne
{
	PhysicsEnigne(size_t expectedDynamicBodies = 200);

	int64_t AddBody(
		const BodyProperties& props,
		ShapeType shapeType,
		DirectX::XMFLOAT3 scales,
		bool isDynamic, 
		uint64_t* bodyId,
		DirectX::XMFLOAT3 constForce = {0, -2.8, 0});
	
	int64_t GetTransformMatrixForBody(
		uint64_t bodyId,
		DirectX::XMFLOAT4X4* mat);

	int64_t UpdateBodies(float dt);

private:

	int64_t FindIntersections(
		float dt);

	Body* GetBody(
		uint64_t bodyId);

	Shape CreateDefaultShape(
		ShapeType type,
		DirectX::XMFLOAT3 scales);


private:
	std::vector<Body> staticBodies;
	std::vector<DirectX::XMFLOAT3> constForces; // per dynamic body
	std::vector<Body> dynamicBodies;
	std::vector<Contact> contactPoints;

};

