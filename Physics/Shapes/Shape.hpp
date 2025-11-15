#pragma once
#include <DirectXMath.h>
#include <inttypes.h>


typedef int64_t(*GetTrasformationMatrix)(char* shapeData, DirectX::XMFLOAT4X4* destMat);

struct Shape
{
	GetTrasformationMatrix getTrasformationMatrix;

	char* shapeData;
};