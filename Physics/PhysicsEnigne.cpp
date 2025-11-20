#include "PhysicsEnigne.h"
#include "Shapes//ShapeBox.hpp"
#include <inttypes.h>
#include "Intersection.hpp"

using namespace std;
using namespace DirectX;

#define GET_BODY(id) if((id & STATIC_FLAG > 0) {staticBodies[(id & ~STATIC_FLAG) - 1];}else {dynamicBodies[(id & ~STATIC_FLAG) - 1];}
constexpr static uint64_t STATIC_FLAG = 0x01ULL << 63;

PhysicsEnigne::PhysicsEnigne(
	size_t expectedDynamicBodies)
{
	// some arbitrary value, can be changed
	contactPoints.resize(expectedDynamicBodies * expectedDynamicBodies * 2);
}

int64_t PhysicsEnigne::FindIntersections(float dt)
{
	size_t detectedIntersections = 0;
	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{

		for (size_t j = i + 1; j < dynamicBodies.size(); j++)
		{
			if (CheckIntersection(&dynamicBodies[i], &dynamicBodies[j], &contactPoints[detectedIntersections], dt))
			{
				detectedIntersections++;
				if (detectedIntersections >= contactPoints.size())
				{
					Contact fill = {};
					contactPoints.resize(contactPoints.size() * 4, fill);
				}
			}
		}

		for (size_t j =0; j < staticBodies.size(); j++)
		{
			if (CheckIntersection(&dynamicBodies[i], &staticBodies[j], &contactPoints[detectedIntersections], dt))
			{
				detectedIntersections++;
				if (detectedIntersections >= contactPoints.size())
				{
					Contact fill = {};
					contactPoints.resize(contactPoints.size() * 4, fill);
				}
			}
		}

	}
	return 0;
}

Body* PhysicsEnigne::GetBody(
	uint64_t bodyId)
{
	if ((bodyId & STATIC_FLAG) > 0)
	{ 
		return &staticBodies[(bodyId & ~STATIC_FLAG) - 1];
	}
	else 
	{ 
		return &dynamicBodies[(bodyId & ~STATIC_FLAG) - 1];
	}
}

Shape PhysicsEnigne::CreateDefaultShape(
	ShapeType type, 
	DirectX::XMFLOAT3 scales)
{
	switch (type)
	{
	case ShapeType::OrientedBox:
		GetDefaultBoxShape(scales);
		break;
	default:
		exit(-1);
	};
}

int64_t PhysicsEnigne::AddBody(
	const BodyProperties& props,
	ShapeType shapeType,
	DirectX::XMFLOAT3 scales,
	bool isDynamic,
	uint64_t* bodyId,
	DirectX::XMFLOAT3 constForce)
{
	Body body;
	body.angVelocity = props.angVelocity;
	body.linVelocity = props.linVelocity;
	body.massInv = props.massInv;
	body.position = props.position;
	body.rotation = props.rotation;
	body.shape = CreateDefaultShape(shapeType, scales);

	if (isDynamic)
	{
		constForces.push_back(constForce);
		dynamicBodies.push_back(body);
		*bodyId = dynamicBodies.size();
		return 0;
	}
	else
	{
		staticBodies.push_back(body);
		*bodyId = staticBodies.size();
		*bodyId |= STATIC_FLAG;
		return 0;
	}
	return 1;
}

int64_t PhysicsEnigne::GetTransformMatrixForBody(
	uint64_t bodyId, 
	DirectX::XMFLOAT4X4* mat)
{
	const Body* body = GetBody(bodyId);
	body->shape.getTrasformationMatrix(body->shape.shapeData, mat);
	XMMATRIX transform = XMLoadFloat4x4(mat);
	transform = transform * XMMatrixTranslation(body->position.x, body->position.y, body->position.z);
	XMStoreFloat4x4(mat, transform);
	return 0;
}

int64_t PhysicsEnigne::UpdateBodies(float dt)
{
	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		Body* body = &dynamicBodies[i];
		float mass = 1.0f / body->massInv;

		XMVECTOR constForce = XMLoadFloat3(&constForces[i]);
		XMVECTOR Impulse = constForce * mass * dt;
		XMVECTOR dv = Impulse * body->massInv;
		XMVECTOR v = XMLoadFloat3(&body->linVelocity) + dv;
		XMStoreFloat3(&body->linVelocity, v);
	}

	FindIntersections(dt);

	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		UpdateBody(&dynamicBodies[i], dt);
	}


	return 0;
}
