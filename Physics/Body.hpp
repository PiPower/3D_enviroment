#include <DirectXMath.h>
#include "./Shapes/Shape.hpp"
enum class ShapeType
{
	OrientedBox, // box constructed by combining scaled vectors [1,0,0],[0,1,0],[0,0,1]
	Sphere
};



struct BodyProperties
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 linVelocity;
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 angVelocity;
	float massInv;
};

struct Body
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 linVelocity;
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 angVelocity;
	float massInv;

	Shape shape;
};