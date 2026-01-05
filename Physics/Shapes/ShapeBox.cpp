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
	const DirectX::XMFLOAT4* rotQuat,
	DirectX::XMFLOAT3* supportVec, 
	float bias)
{
	Box* box = (Box*)shape->shapeData;

	XMVECTOR dirVec = XMLoadFloat3(dir);
	XMVECTOR posVec = XMLoadFloat3(pos);
	XMVECTOR rotationQuat = XMLoadFloat4(rotQuat);
	XMMATRIX rotMat = XMMatrixTranspose(XMMatrixRotationQuaternion(rotationQuat));
	XMVECTOR vert = XMVector3Transform(XMLoadFloat3(&box->vertecies[0]), rotMat) + posVec;
	XMStoreFloat3(supportVec, vert);
	
	float maxProd;
	XMStoreFloat(&maxProd, XMVector3Dot(vert, dirVec));
	for (uint8_t i = 1; i < 8; i++)
	{
		vert = XMVector3Transform(XMLoadFloat3(&box->vertecies[i]), rotMat);
		vert = vert + posVec;
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
	const float dy = 2.0f * box->scales.y;
	const float dx = 2.0f * box->scales.x;
	const float dz = 2.0f * box->scales.y;

	inertiaTensor->_11 = 12.0f * invMass * 1 / (dy * dy + dz * dz);
	inertiaTensor->_22 = 12.0f * invMass * 1 / (dx * dx + dz * dz);
	inertiaTensor->_33 = 12.0f * invMass * 1 / (dy * dy + dx * dx);
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
	XMMATRIX rotationMat = XMMatrixTranspose(XMMatrixRotationQuaternion(XMLoadFloat4(rotationQuat)));
	tensor = rotationMat * tensor * XMMatrixTranspose(rotationMat);
	XMStoreFloat4x4(inertiaTensor, tensor);
}

static void GetPartialInertiaTensorBox(
	const Shape* shape,
	DirectX::XMFLOAT4X4* inertiaTensor)
{
	Box* box = (Box*)shape->shapeData;
	memset(inertiaTensor, 0, sizeof(XMFLOAT4X4));

	const float dy = 2.0f * box->scales.y;
	const float dx = 2.0f * box->scales.x;
	const float dz = 2.0f * box->scales.y;

	inertiaTensor->_11 = (dy * dy + dz * dz)/12.0f;
	inertiaTensor->_22 = (dx * dx +	dz * dz)/12.0f;
	inertiaTensor->_33 = (dy * dy + dx * dx)/12.0f;
	inertiaTensor->_44 = 1.0f;
}

static void GetCenterOfMassBox(
	const Shape* shape,
	XMFLOAT3* CoM)
{
	CoM->x = 0;
	CoM->y = 0;
	CoM->z = 0;
}

static BoundingBox GetBoundingBox_Box(
	const Shape* shape,
	const DirectX::XMFLOAT3* position,
	const DirectX::XMFLOAT4* rotationQuat)
{
	Box* box = (Box*)shape->shapeData;

	XMMATRIX rotationMat = XMMatrixTranspose(XMMatrixRotationQuaternion(XMLoadFloat4(rotationQuat)));
	XMVECTOR pos = XMLoadFloat3(position);
	BoundingBox bBox;
	for (uint8_t i = 0; i < 8; i++)
	{
		XMFLOAT3 vertex;
		XMStoreFloat3(&vertex, XMVector3Transform(XMLoadFloat3(&box->vertecies[i]), rotationMat) + pos);
		bBox.Expand(vertex);
	}

	return bBox;
}

static void GetFaceNormalFromPoint_Box(
	const Shape* shape,
	const DirectX::XMFLOAT3* pointOnShape,
	DirectX::XMFLOAT3* normal)
{
	Box* box = (Box*)shape->shapeData;
	constexpr XMFLOAT3 normals[6] = { {0, 1, 0}, {0, -1, 0}, 
									  {1, 0, 0}, {-1, 0, 0},
									  {0, 0, 1}, {0, 0, -1} };

	XMFLOAT3 pointOnPlane[6] = { {0, box->scales.y, 0}, {0, -box->scales.y, 0},
								 {box->scales.x, 0, 0}, {-box->scales.x, 0, 0},
								 {0, 0, box->scales.z}, {0, 0, -box->scales.z}};

	float minProd = 1e10;
	XMVECTOR v_point = XMLoadFloat3(pointOnShape);
	for (int i = 0; i < 6; i++)
	{
		XMVECTOR v_vec = XMVector3Normalize(v_point - XMLoadFloat3(&pointOnPlane[i]));
		float prod;
		XMStoreFloat(&prod, XMVector3Dot(v_vec, XMLoadFloat3(&normals[i])));
		if (fabsf(prod) < minProd)
		{
			minProd = fabsf(prod);
			*normal = normals[i];
		}
	}
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
	boxShape.getPartialInertiaTensor = GetPartialInertiaTensorBox;
	boxShape.getBoundingBox = GetBoundingBox_Box;
	boxShape.getFaceNormalFromPoint = GetFaceNormalFromPoint_Box;

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
