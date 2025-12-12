#include "ShapeBox.hpp"
#include <cstring>
using namespace DirectX;

/*
	vertex name convention -> (front | back) + (left | right) + (top | back)
*/
constexpr static uint8_t frt = 0;
constexpr static uint8_t flt = 1;
constexpr static uint8_t flb = 2;
constexpr static uint8_t frb = 3;

constexpr static uint8_t brt = 4; 
constexpr static uint8_t blt = 5; 
constexpr static uint8_t blb = 6;
constexpr static uint8_t brb = 7;
constexpr static uint8_t VertexCount = 8;

struct Box
{
	XMFLOAT3 scales;
	// 4 front vertecies
	XMFLOAT3 vertecies[VertexCount];
};

static int64_t TransformationMatrix(
	char* shapeData,
	DirectX::XMFLOAT4X4* destMat)
{
	Box* box = (Box*)shapeData;
	XMMATRIX scaleMatrix = XMMatrixScaling(box->scales.x, box->scales.y, box->scales.z);
	XMStoreFloat4x4(destMat, scaleMatrix);
	return 0;
}

static void SupportFn(
	const Shape* shape,
	const DirectX::XMFLOAT3* pos,
	const DirectX::XMFLOAT3* dir, 
	DirectX::XMFLOAT3* supportVec, 
	float bias)
{
	Box* box = (Box*)shape->shapeData;

	XMVECTOR dirVec = XMLoadFloat3(dir);
	XMVECTOR posVec = XMLoadFloat3(pos);
	XMVECTOR vert = XMLoadFloat3(&box->vertecies[0]) + posVec;
	XMStoreFloat3(supportVec, vert);

	float maxProd;
	XMStoreFloat(&maxProd, XMVector3Dot(vert, dirVec));
	for (uint8_t i = 1; i < 8; i++)
	{
		vert = XMLoadFloat3(&box->vertecies[i]) + posVec;
		float prod;
		XMStoreFloat(&prod, XMVector3Dot(vert, dirVec));
		if (prod > maxProd)
		{
			maxProd = prod;
			XMStoreFloat3(supportVec, vert);
		}
	}
	XMStoreFloat3(supportVec, XMLoadFloat3(supportVec) + XMVector3Normalize(dirVec) * bias);
}

static void GetInverseInertiaTensorBox(
	const Shape* shape,
	float invMass,
	DirectX::XMFLOAT4X4* inertiaTensor)
{
	Box* box = (Box*)shape->shapeData;
	memset(inertiaTensor, 0, sizeof(XMFLOAT4X4));
	inertiaTensor->_11 = 12.0f * invMass * 1 / (box->scales.y * box->scales.y + box->scales.z * box->scales.z);
	inertiaTensor->_22 = 12.0f * invMass * 1 / (box->scales.x * box->scales.x + box->scales.z * box->scales.z);
	inertiaTensor->_33 = 12.0f * invMass * 1 / (box->scales.y * box->scales.y + box->scales.x * box->scales.x);
	inertiaTensor->_44 = 1.0f;
}

static void GetInverseInertiaTensorWorldSpaceBox(
	const Shape* shape,
	float invMass,
	const DirectX::XMFLOAT4* rotationQuat,
	DirectX::XMFLOAT4X4* inertiaTensor)
{
	shape->getInverseInertiaTensor(shape, invMass, inertiaTensor);
	XMMATRIX tensor = XMLoadFloat4x4(inertiaTensor);
	XMMATRIX rotationMat = XMMatrixRotationQuaternion(XMLoadFloat4(rotationQuat));
	tensor = rotationMat * tensor * XMMatrixTranspose(rotationMat);
	XMStoreFloat4x4(inertiaTensor, tensor);
}

static void GetCenterOfMassBox(
	const Shape* shape,
	XMFLOAT3* CoM)
{

}

Shape GetDefaultBoxShape(
	DirectX::XMFLOAT3 scales)
{
	Shape boxShape;
	boxShape.getTrasformationMatrix = TransformationMatrix;
	boxShape.supportFunction = SupportFn;
	boxShape.getInverseInertiaTensor = GetInverseInertiaTensorBox;
	boxShape.getInverseInertiaTensorWorldSpace = GetInverseInertiaTensorWorldSpaceBox;
	boxShape.getCenterOfMass = GetCenterOfMassBox;

	Box* box = new Box();
	box->scales = scales;

	box->vertecies[frt] = { 1.0f, 1.0f, -1.0f };
	box->vertecies[flt] = { -1.0f, 1.0f, -1.0f };
	box->vertecies[flb] = { -1.0f, -1.0f, -1.0f };
	box->vertecies[frb] = { 1.0f, -1.0f, -1.0f };

	box->vertecies[brt] = { 1.0f, 1.0f, 1.0f };
	box->vertecies[blt] = { -1.0f, 1.0f, 1.0f };
	box->vertecies[blb] = { -1.0f, -1.0f, 1.0f };
	box->vertecies[brb] = { 1.0f, -1.0f, 1.0f };


	XMMATRIX scaleMatrix = XMMatrixScaling(scales.x, scales.y, scales.z);
	for (uint8_t i = 0; i < VertexCount; i++)
	{
		XMVECTOR vertex = XMLoadFloat3(&box->vertecies[i]);
		XMStoreFloat3(&box->vertecies[i], XMVector3Transform(vertex, scaleMatrix));
	}

	boxShape.shapeData = (char*)box;
	return boxShape;
}
