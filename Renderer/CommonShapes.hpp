#pragma once
#include <DirectXMath.h>
#include <vector>

struct CommonVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 tex;
};

struct Geometry
{
    const std::vector<CommonVertex>* const vertecies;
    const std::vector<uint16_t>* const indicies;
};

extern const Geometry commonBox;