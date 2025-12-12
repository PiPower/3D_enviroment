#pragma once
#include <DirectXMath.h>
#include <inttypes.h>

struct Shape;

typedef int64_t(*GetTrasformationMatrix)(
	char* shapeData,
	DirectX::XMFLOAT4X4* destMat);

// dir MUST BE normalized to 1
typedef void(*SupportFunction)(
	const Shape* shape,
	const DirectX::XMFLOAT3* pos,
	const DirectX::XMFLOAT3* dir, 
	DirectX::XMFLOAT3* supportVec, 
	float bias);

// dir MUST BE normalized to 1
typedef void(*GetInverseInertiaTensor)(
	const Shape* shape,
	float invMass,
	DirectX::XMFLOAT4X4* inertiaTensor);

typedef void(*GetInverseInertiaTensorWorldSpace)(
	const Shape* shape,
	float invMass,
	const DirectX::XMFLOAT4* rotationQuat,
	DirectX::XMFLOAT4X4* inertiaTensor);

typedef void(*GetCenterOfMass)(
	const Shape* shape,
	DirectX::XMFLOAT3* CoM);

struct Shape
{
	GetTrasformationMatrix getTrasformationMatrix;
	SupportFunction supportFunction;
	GetInverseInertiaTensor getInverseInertiaTensor;
	GetInverseInertiaTensorWorldSpace getInverseInertiaTensorWorldSpace;
	GetCenterOfMass getCenterOfMass;
	char* shapeData;
};