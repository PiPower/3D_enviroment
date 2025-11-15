#include "PhysicsEnigne.h"
#include "Shapes//ShapeBox.hpp"
#include <inttypes.h>
using namespace std;
using namespace DirectX;

#define GET_BODY(id) if((id & STATIC_FLAG > 0) {staticBodies[(id & ~STATIC_FLAG) - 1];}else {dynamicBodies[(id & ~STATIC_FLAG) - 1];}
constexpr static uint64_t STATIC_FLAG = 0x01ULL << 63;
struct BoxDescirptor
{
	XMFLOAT3 front;
};

PhysicsEnigne::PhysicsEnigne()
{
}

Body* PhysicsEnigne::GetBody(uint64_t bodyId)
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

Shape PhysicsEnigne::CreateDefaultShape(ShapeType type, DirectX::XMFLOAT3 scales)
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
	DirectX::XMFLOAT3 defaultForce)
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
		defaultForces.push_back(defaultForce);
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
