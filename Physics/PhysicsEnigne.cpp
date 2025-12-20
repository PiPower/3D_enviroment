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
		return GetDefaultBoxShape(scales);
		break;
	default:
		exit(-1);
		break;
	};
}

void PhysicsEnigne::ResolveContact(
	Contact* contact)
{
	Body* bodyA = contact->bodyA;
	Body* bodyB= contact->bodyB;

	float elastictyFactor =  bodyA->elasticity * bodyB->elasticity;

	XMFLOAT3 angularImpulseA;
	XMFLOAT3 angularImpulseB;
	float angularFactor;
	GetAngularImpulse(bodyA, &contact->ptOnA, &contact->normal, &angularImpulseA);
	GetAngularImpulse(bodyB, &contact->ptOnB, &contact->normal, &angularImpulseB);
	XMVECTOR totalAngularImpulse = XMLoadFloat3(&angularImpulseA) + XMLoadFloat3(&angularImpulseB);
	XMStoreFloat(&angularFactor, XMVector3Dot(totalAngularImpulse, XMLoadFloat3(&contact->normal)));
	float denominator = 1.0f / (bodyA->massInv + bodyB->massInv + angularFactor);

	XMFLOAT3 CoM;
	bodyA->GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_velA = XMLoadFloat3(&bodyA->linVelocity) + XMVector3Cross(XMLoadFloat3(&contact->ptOnA) - XMLoadFloat3(&CoM), XMLoadFloat3(&bodyA->angVelocity));
	bodyB->GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_velB = XMLoadFloat3(&bodyB->linVelocity) + XMVector3Cross(XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&CoM), XMLoadFloat3(&bodyB->angVelocity));
	auto z = v_velA - v_velB;
	XMVECTOR closingSpeed = denominator * (1 + elastictyFactor) * XMVector3Dot(v_velA - v_velB, XMLoadFloat3(&contact->normal));
	XMVECTOR reboundImpulse = XMLoadFloat3(&contact->normal) * closingSpeed;

	//XMVECTOR I_A = denominator * (XMLoadFloat3(&bodyB->linVelocity) - XMLoadFloat3(&bodyA->linVelocity));
	//XMVECTOR I_B = -I_A;
	//XMVECTOR totalImpulse = elastictyFactor * (I_A - I_B);
	//XMVECTOR closingSpeed = XMVector3Dot(totalImpulse, XMLoadFloat3(&contact->normal));
	//XMVECTOR reboundImpulse = XMLoadFloat3(&contact->normal) * closingSpeed;

	XMFLOAT3 ImpulseStorage;
	XMStoreFloat3(&ImpulseStorage, -1.0f * reboundImpulse);
	bodyA->ApplyImpulse(&contact->ptOnA, &ImpulseStorage);

	XMStoreFloat3(&ImpulseStorage, reboundImpulse);
	bodyB->ApplyImpulse(&contact->ptOnB, &ImpulseStorage);


	// projecting bodies outside of eachother
	if (contact->timeOfImpact == 0.0f)
	{
		XMVECTOR dist = XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&contact->ptOnA);

		float totalMassInv = 1.0f / (bodyA->massInv + bodyB->massInv);
		XMStoreFloat3(&bodyA->position, XMLoadFloat3(&bodyA->position) + dist * bodyA->massInv / totalMassInv);
		XMStoreFloat3(&bodyB->position, XMLoadFloat3(&bodyB->position) + dist * bodyB->massInv / totalMassInv);
	}
	
}

void PhysicsEnigne::GetAngularImpulse(
	const Body* body, 
	const DirectX::XMFLOAT3* point,
	const DirectX::XMFLOAT3* normal, 
	DirectX::XMFLOAT3* Impulse)
{
	XMFLOAT4X4 inertiaTensor;
	body->GetInverseInertiaTensorWorldSpace(&inertiaTensor);

	XMFLOAT3 CoM;
	body->GetCenterOfMassWorldSpace(&CoM);

	XMVECTOR r = XMLoadFloat3(point) - XMLoadFloat3(&CoM);
	XMVECTOR n = XMLoadFloat3(normal);
	XMVECTOR prod =  XMVector3Transform(XMVector3Cross(n, r), XMLoadFloat4x4(&inertiaTensor));
	XMStoreFloat3(Impulse, XMVector3Cross(r, prod));
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
	body.elasticity = props.elasticity;
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

	XMMATRIX translation = XMMatrixTranslation(body->position.x, body->position.y, body->position.z);
	XMMATRIX rotation = XMMatrixTranspose(XMMatrixRotationQuaternion(XMLoadFloat4(&body->rotation)));
	XMMATRIX transform = XMLoadFloat4x4(mat) *
						 rotation *
						 translation;
						

	XMStoreFloat4x4(mat, transform);
	return 0;
}

int64_t PhysicsEnigne::UpdateBodies(float dt)
{
	static float time;
	time += dt;
	if (time >= 1.07)
	{
		int x = 2;
	}
	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		Body* body = &dynamicBodies[i];
		float mass = 1.0f / body->massInv;

		XMVECTOR v_constForce = XMLoadFloat3(&constForces[i]);
		XMVECTOR v_Impulse = v_constForce * mass * dt;
		XMFLOAT3 Impulse;
		XMStoreFloat3(&Impulse, v_Impulse);
		body->ApplyLinearImpulse(&Impulse);

	}
	
	FindIntersections(dt);
	sort(contactPoints.begin(), contactPoints.begin() + detectedIntersections, [](const Contact& l, const Contact& r)
		{
			return l.timeOfImpact < r.timeOfImpact;
		}
	);

	float accumulatedTime = 0.0f;
	for (size_t i = 0; i < detectedIntersections; i++)
	{
		Contact& contact = contactPoints[i];
		const float dt_c = contact.timeOfImpact - accumulatedTime;

		static int tick;
		if (tick == 8)
		{
			int x = 2;
		}

		for (size_t i = 0; i < dynamicBodies.size(); i++)
		{
			dynamicBodies[i].UpdateBody(dt_c);
		}


		ResolveContact(&contactPoints[i]);
		accumulatedTime += dt_c;
	}

	const float timeRemaining = dt - accumulatedTime;
	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		dynamicBodies[i].UpdateBody(dt);
	}
	return 0;
}
