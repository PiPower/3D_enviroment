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


struct Shape
{
	GetTrasformationMatrix getTrasformationMatrix;
	SupportFunction supportFunction;
	char* shapeData;
};