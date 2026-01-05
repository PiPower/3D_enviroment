#pragma once
#include <DirectXMath.h>
#include "../BoundingBox.hpp"
#include <inttypes.h>

struct Shape;

typedef int64_t(*GetTrasformationMatrix)(
	char* shapeData,
	DirectX::XMFLOAT4X4* destMat);

typedef void(*SupportFunction)(
	const Shape* shape,
	const DirectX::XMFLOAT3* pos,
	const DirectX::XMFLOAT3* dir, 
	const DirectX::XMFLOAT4* rotQuat,
	DirectX::XMFLOAT3* supportVec, 
	float bias);

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


typedef void(*GetPartialInertiaTensor)(
	const Shape* shape,
	DirectX::XMFLOAT4X4* inertiaTensor);

typedef BoundingBox (*GetBoundingBox)(
	const Shape* shape,
	const DirectX::XMFLOAT3* position,
	const DirectX::XMFLOAT4* rotationQuat);

// function DOES NOT check if pointOnShape lays on the shape
typedef void(*GetFaceNormalFromPoint)(
	const Shape* shape,
	const DirectX::XMFLOAT3* pointOnShape,
	DirectX::XMFLOAT3* normal);

struct Shape
{
	GetTrasformationMatrix getTrasformationMatrix;
	SupportFunction supportFunction;
	GetInverseInertiaTensor getInverseInertiaTensor;
	GetInverseInertiaTensorWorldSpace getInverseInertiaTensorWorldSpace;
	GetCenterOfMass getCenterOfMass;
	GetPartialInertiaTensor getPartialInertiaTensor;
	GetBoundingBox getBoundingBox;
	GetFaceNormalFromPoint getFaceNormalFromPoint;
	char* shapeData;
};