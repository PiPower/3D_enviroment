#pragma once
#include <vector>
#include "Body.hpp"

struct PhysicsEnigne
{
	PhysicsEnigne();

	int64_t AddBody(
		const BodyProperties& props,
		ShapeType shapeType,
		DirectX::XMFLOAT3 scales,
		bool isDynamic, 
		uint64_t* bodyId,
		DirectX::XMFLOAT3 defaultForce = {0, 9.8, 0});
	
	int64_t GetTransformMatrixForBody(
		uint64_t bodyId,
		DirectX::XMFLOAT4X4* mat);

private:
	Body* GetBody(
		uint64_t bodyId);

	Shape CreateDefaultShape(ShapeType type, DirectX::XMFLOAT3 scales);

private:
	std::vector<Body> staticBodies;
	std::vector<DirectX::XMFLOAT3> defaultForces; // per dynamic body
	std::vector<Body> dynamicBodies;

};

