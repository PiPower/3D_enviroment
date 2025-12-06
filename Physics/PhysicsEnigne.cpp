#include "PhysicsEnigne.h"
#include "Shapes//ShapeBox.hpp"
#include <inttypes.h>
#include "Intersection.hpp"
#include <algorithm>

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
	detectedIntersections = 0;
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

void PhysicsEnigne::ResolveContact(
	Contact* contact)
{
	Body* bodyA = contact->bodyA;
	Body* bodyB= contact->bodyB;

	float totalMassInv = 1.0f/(bodyA->massInv + bodyB->massInv);

	XMVECTOR I_A = totalMassInv * (XMLoadFloat3(&bodyB->linVelocity) - XMLoadFloat3(&bodyA->linVelocity));
	XMVECTOR I_B = -I_A;
	XMVECTOR totalImpulse = I_A - I_B;
	XMVECTOR closingSpeed = XMVector3Dot(I_A - I_B, XMLoadFloat3(&contact->normal));
	XMVECTOR yolo = -2.0f * XMVector3Dot(XMLoadFloat3(&bodyA->linVelocity) - XMLoadFloat3(&bodyB->linVelocity), XMLoadFloat3(&contact->normal)) / (bodyA->massInv + bodyB->massInv);
	XMVECTOR reboundImpulse = XMLoadFloat3(&contact->normal) * closingSpeed;

	XMFLOAT3 ImpulseStorage;
	XMStoreFloat3(&ImpulseStorage, reboundImpulse);
	bodyA->ApplyLinearImpulse(&ImpulseStorage);

	XMStoreFloat3(&ImpulseStorage, -reboundImpulse);
	bodyB->ApplyLinearImpulse(&ImpulseStorage);


	// projecting bodies outside of eachother
	if (contact->timeOfImpact == 0.0f)
	{
		XMVECTOR dist = XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&contact->ptOnA);

		XMStoreFloat3(&bodyA->position, XMLoadFloat3(&bodyA->position) + dist * bodyA->massInv / totalMassInv);
		XMStoreFloat3(&bodyB->position, XMLoadFloat3(&bodyB->position) + dist * bodyB->massInv / totalMassInv);
	}
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
		body.massInv = 0.0f;
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
	sort(contactPoints.begin(), contactPoints.begin() + detectedIntersections, [](const Contact& l, const Contact& r)
		{
			return l.timeOfImpact < r.timeOfImpact;
		}
	);

	for (size_t i = 0; i < detectedIntersections; i++)
	{
		Contact& contact = contactPoints[i];
		ResolveContact(&contactPoints[i]);
	}

	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		dynamicBodies[i].UpdateBody(dt);
	}

	return 0;
}
