#include "PhysicsEnigne.h"
#include "Shapes//ShapeBox.hpp"
#include <inttypes.h>
#include "Intersection.hpp"
#include <algorithm>
using namespace std;
using namespace DirectX;

#define GET_BODY(id) if((id & STATIC_FLAG > 0) {staticBodies[(id & ~STATIC_FLAG) - 1];}else {dynamicBodies[(id & ~STATIC_FLAG) - 1];}
constexpr static uint64_t BODY_STATIC_FLAG = 0x01ULL << 63;

PhysicsEnigne::PhysicsEnigne(
	size_t expectedDynamicBodies,
	size_t expectedStaticBodies)
{
	// some arbitrary value, can be changed
	contactPoints.resize(expectedDynamicBodies * expectedDynamicBodies * 2);
	sortedBodies.resize((expectedDynamicBodies + expectedStaticBodies) * 2);
	collisionPairs.resize((expectedDynamicBodies + expectedStaticBodies) * 2);
}

int64_t PhysicsEnigne::FindIntersections(float dt)
{
	contactPoints.clear();
	contactPoints.push_back({});
	for (size_t i = 0; i < collisionPairs.size(); i++)
	{
		Body* bodyA = GetBody(collisionPairs[i].idA);
		Body* bodyB = GetBody(collisionPairs[i].idB);

		if (CheckIntersection(bodyA, bodyB, &contactPoints[contactPoints.size() - 1], dt))
		{
			contactPoints.push_back({});
		}
	
	}
	return 0;
}

void PhysicsEnigne::AddForce(
	uint64_t bodyId, 
	uint8_t	forceComponent,
	const DirectX::XMFLOAT3& Force)
{
	if ((bodyId & BODY_STATIC_FLAG) > 0)
	{
		return;
	}

	XMFLOAT3 force;
	XMStoreFloat3(&force, XMLoadFloat3(&dynamicForces[bodyId - 1]) + XMLoadFloat3(&Force));

	if ((forceComponent & X_COMPONENT) > 0) { dynamicForces[bodyId - 1].x = force.x; }
	if ((forceComponent & Y_COMPONENT) > 0) { dynamicForces[bodyId - 1].y = force.y; }
	if ((forceComponent & Z_COMPONENT) > 0) { dynamicForces[bodyId - 1].z = force.z; }

	return;
}

void PhysicsEnigne::SetLinearVelocity(
	uint64_t bodyId,
	uint8_t	velocityComponent,
	const DirectX::XMFLOAT3& v)
{
	if ((bodyId & BODY_STATIC_FLAG) > 0)
	{
		return;
	}

	if ((velocityComponent & X_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.x = v.x; }
	if ((velocityComponent & Y_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.y = v.y; }
	if ((velocityComponent & Z_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.z = v.z; }
	return;
}

void PhysicsEnigne::GetLinearVelocity(
	uint64_t bodyId, 
	DirectX::XMFLOAT3* v)
{
	if ((bodyId & BODY_STATIC_FLAG) > 0)
	{
		return;
	}
	*v = dynamicBodies[bodyId - 1].linVelocity;
}

void PhysicsEnigne::AddLinearVelocity(
	uint64_t bodyId,
	uint8_t velocityComponent, 
	const DirectX::XMFLOAT3& v)
{
	if ((bodyId & BODY_STATIC_FLAG) > 0)
	{
		return;
	}

	if ((velocityComponent & X_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.x += v.x; }
	if ((velocityComponent & Y_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.y += v.y; }
	if ((velocityComponent & Z_COMPONENT) > 0) { dynamicBodies[bodyId - 1].linVelocity.z += v.z; }
	return;
}

void PhysicsEnigne::GetDistanceBetweenBodies(
	uint64_t idBodyA,
	uint64_t idBodyB,
	float* dist)
{
	DistanceBetweenBodies(GetBody(idBodyA), GetBody(idBodyB), nullptr, nullptr, dist);
}

void PhysicsEnigne::GetDistanceBetweenBodies(
	uint64_t idBodyA,
	uint64_t idBodyB,
	DirectX::XMFLOAT3* ptOnA,
	DirectX::XMFLOAT3* ptOnB,
	float* dist)
{
	DistanceBetweenBodies(GetBody(idBodyA), GetBody(idBodyB), ptOnA, ptOnB, dist);
}

Body* PhysicsEnigne::GetBody(
	uint64_t bodyId)
{
	if ((bodyId & BODY_STATIC_FLAG) > 0)
	{ 
		return &staticBodies[(bodyId & ~BODY_STATIC_FLAG) - 1];
	}
	else 
	{ 
		return &dynamicBodies[bodyId - 1];
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
	XMVECTOR v_normal = XMLoadFloat3(&contact->normal);
	float elastictyFactor =  bodyA->elasticity * bodyB->elasticity;

	XMFLOAT3 angularImpulseA;
	XMFLOAT3 angularImpulseB;
	float angularFactor;
	GetAngularImpulse(bodyA, &contact->ptOnA, &contact->normal, &angularImpulseA);
	GetAngularImpulse(bodyB, &contact->ptOnB, &contact->normal, &angularImpulseB);
	XMVECTOR totalAngularImpulse = XMLoadFloat3(&angularImpulseA) + XMLoadFloat3(&angularImpulseB);
	XMStoreFloat(&angularFactor, XMVector3Dot(totalAngularImpulse, v_normal));
	float denominator = 1.0f / (bodyA->massInv + bodyB->massInv + angularFactor);

	XMFLOAT3 CoM;
	bodyA->GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_velA = XMLoadFloat3(&bodyA->linVelocity) + XMVector3Cross(XMLoadFloat3(&contact->ptOnA) - XMLoadFloat3(&CoM), XMLoadFloat3(&bodyA->angVelocity));
	bodyB->GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_velB = XMLoadFloat3(&bodyB->linVelocity) + XMVector3Cross(XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&CoM), XMLoadFloat3(&bodyB->angVelocity));
	XMVECTOR closingSpeed = denominator * (1 + elastictyFactor) * XMVector3Dot(v_velA - v_velB, v_normal);
	XMVECTOR reboundImpulse = v_normal * closingSpeed;

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


	float frictionCoeff = bodyA->friction * bodyB->friction;
	XMVECTOR v_velProjection = v_normal * XMVector3Dot(v_velA - v_velB, v_normal);
	XMVECTOR v_velTang = v_velA - v_velB - v_velProjection;

	XMFLOAT3 angularImpulseFrictionA;
	XMFLOAT3 angularImpulseFrictionB;
	XMFLOAT3 v_velTangNorm;
	XMStoreFloat3(&v_velTangNorm, XMVector3Normalize(v_velTang));

	GetAngularImpulse(bodyA, &contact->ptOnA, &v_velTangNorm, &angularImpulseFrictionA);
	GetAngularImpulse(bodyB, &contact->ptOnB, &v_velTangNorm, &angularImpulseFrictionB);
	float angularFrictionFactor;
	XMStoreFloat(&angularFrictionFactor,
		XMVector3Dot(XMLoadFloat3(&angularImpulseFrictionA) + XMLoadFloat3(&angularImpulseFrictionB), XMLoadFloat3(&v_velTangNorm)));
	float denominatorFriction = 1.0f / (bodyA->massInv + bodyB->massInv + angularFrictionFactor);

	XMVECTOR ImpulseFriction = denominatorFriction * frictionCoeff * v_velTang;
	XMStoreFloat3(&ImpulseStorage, -1.0f * ImpulseFriction);
	//bodyA->ApplyImpulse(&contact->ptOnA, &ImpulseStorage);

	XMStoreFloat3(&ImpulseStorage, ImpulseFriction);
	//bodyB->ApplyImpulse(&contact->ptOnB, &ImpulseStorage);

	// projecting bodies outside of eachother
	if (contact->timeOfImpact == 0.0f)
	{
		XMVECTOR dist = XMLoadFloat3(&contact->ptOnB) - XMLoadFloat3(&contact->ptOnA);

		float totalMassInv = bodyA->massInv + bodyB->massInv;
		XMStoreFloat3(&bodyA->position, XMLoadFloat3(&bodyA->position) + dist * bodyA->massInv / totalMassInv);
		XMStoreFloat3(&bodyB->position, XMLoadFloat3(&bodyB->position) - dist * bodyB->massInv / totalMassInv);
	}
	
}

void PhysicsEnigne::SortBodiesByDistanceToPlane(
	const DirectX::XMFLOAT3* normal,
	float dt)
{
	XMVECTOR n = XMLoadFloat3(normal);
	size_t i;
	for (i = 0; i < dynamicBodies.size() * 2; i+=2)
	{
		constexpr float eps = 0.01f;
		size_t bodyId = i / 2;
		Body* body = &dynamicBodies[bodyId];
		AddBodyToSortedDistanceList(body, normal, dt, i, bodyId + 1);
	}

	for (size_t j = 0; j < staticBodies.size() * 2; j += 2)
	{
		constexpr float eps = 0.01f;
		size_t bodyIdx = i + j;
		size_t bodyId = (j / 2) | BODY_STATIC_FLAG;
		Body* body = &staticBodies[j/2];
		AddBodyToSortedDistanceList(body, normal, dt, bodyIdx, bodyId + 1);
	}

	size_t bodyCount = (dynamicBodies.size() + staticBodies.size());
	std::sort(sortedBodies.begin(), sortedBodies.begin() + bodyCount * 2,
		[](const BodyPlaneDistance& l, const BodyPlaneDistance& r) { return l.distance < r.distance; });
}

void PhysicsEnigne::BroadPhase(float dt)
{
	XMFLOAT3 normal = { 1.0f, 1.0f, 1.0f };
	SortBodiesByDistanceToPlane(&normal, dt);
	BuildCollisionPairs();
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

void PhysicsEnigne::AddBodyToSortedDistanceList(
	const Body* body,
	const DirectX::XMFLOAT3* normal,
	float dt,
	size_t bodyIdx,
	size_t bodyId)
{
	constexpr float eps = 0.01f;
	BoundingBox bBox = body->getBoundingBox();
	XMFLOAT3 expansion;
	XMVECTOR n = XMLoadFloat3(normal);

	XMStoreFloat3(&expansion,
		XMLoadFloat3(&bBox.maxC) + XMLoadFloat3(&body->linVelocity) * dt);
	bBox.Expand(expansion);
	XMStoreFloat3(&expansion,
		XMLoadFloat3(&bBox.maxC) + 1.0f * XMVectorSet(eps, eps, eps, 0));
	bBox.Expand(expansion);

	XMStoreFloat3(&expansion,
		XMLoadFloat3(&bBox.minC) + XMLoadFloat3(&body->linVelocity) * dt);
	bBox.Expand(expansion);
	XMStoreFloat3(&expansion,
		XMLoadFloat3(&bBox.minC) - 1.0f * XMVectorSet(eps, eps, eps, 0));
	bBox.Expand(expansion);

	sortedBodies[bodyIdx].bodyId = bodyId;
	sortedBodies[bodyIdx].isMin = true;
	XMStoreFloat(&sortedBodies[bodyIdx].distance, XMVector3Dot(n, XMLoadFloat3(&bBox.minC)));

	sortedBodies[bodyIdx + 1].bodyId = bodyId;
	sortedBodies[bodyIdx + 1].isMin = false;
	XMStoreFloat(&sortedBodies[bodyIdx + 1].distance, XMVector3Dot(n, XMLoadFloat3(&bBox.maxC)));

}

void PhysicsEnigne::BuildCollisionPairs()
{
	collisionPairs.clear();
	size_t bodyCount = (dynamicBodies.size() + staticBodies.size());
	// Now that the bodies are sorted, build the collision pairs
	for (int i = 0; i < bodyCount * 2; i++) 
	{
		const BodyPlaneDistance& a = sortedBodies[i];
		if (!a.isMin) 
		{
			continue;
		}

		CollisionPair pair;
		pair.idA = a.bodyId;

		for (int j = i + 1; j < bodyCount * 2; j++)
		{
			const BodyPlaneDistance& b = sortedBodies[j];
			// if we've hit the end of the a element, then we're done creating pairs with a
			if (b.bodyId == a.bodyId )
			{
				break;
			}

			if (!b.isMin || (b.bodyId & BODY_STATIC_FLAG) > 0)
			{
				continue;
			}

			pair.idB = b.bodyId;
			collisionPairs.push_back(pair);
		}
	}

}



int64_t PhysicsEnigne::AddBody(
	const BodyProperties& props,
	ShapeType shapeType,
	const DirectX::XMFLOAT3& scales,
	bool isDynamic,
	uint64_t* bodyId,
	bool allowAngularImpulse,
	const LinearVelocityBounds& vBounds,
	const DirectX::XMFLOAT3& constForce)
{
	Body body;
	body.angVelocity = props.angVelocity;
	body.linVelocity = props.linVelocity;
	body.massInv = props.massInv;
	body.position = props.position;
	body.rotation = props.rotation;
	body.elasticity = props.elasticity;
	body.allowAngularImpulse = allowAngularImpulse;
	body.vBounds = vBounds;
	body.friction = props.friction;
	body.shape = CreateDefaultShape(shapeType, scales);

	if (isDynamic)
	{
		constForces.push_back(constForce);
		dynamicForces.push_back({ 0, 0, 0 });
		dynamicBodies.push_back(body);
		*bodyId = dynamicBodies.size();
		return 0;
	}
	else
	{
		body.massInv = 0.0f;
		staticBodies.push_back(body);
		*bodyId = staticBodies.size();
		*bodyId |= BODY_STATIC_FLAG;
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

	for (size_t i = 0; i < dynamicBodies.size(); i++)
	{
		Body* body = &dynamicBodies[i];
		float mass = 1.0f / body->massInv;

		XMVECTOR v_constForce = XMLoadFloat3(&constForces[i]);
		XMVECTOR v_dynamicForce = XMLoadFloat3(&dynamicForces[i]);
		XMVECTOR v_Impulse = (v_constForce + v_dynamicForce) * mass * dt;
		XMFLOAT3 Impulse;
		XMStoreFloat3(&Impulse, v_Impulse);
		body->ApplyLinearImpulse(&Impulse);
	
		dynamicForces[i] = { .0f, .0f, .0f };
	}

	BroadPhase(dt);

	FindIntersections(dt);
	sort(contactPoints.begin(), contactPoints.end() - 1, [](const Contact& l, const Contact& r)
		{
			return l.timeOfImpact < r.timeOfImpact;
		}
	);

	float accumulatedTime = 0.0f;
	for (size_t i = 0; i < contactPoints.size() - 1; i++)
	{
		Contact& contact = contactPoints[i];
		const float dt_c = contact.timeOfImpact - accumulatedTime;

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
