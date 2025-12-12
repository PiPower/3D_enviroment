#pragma once
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
	float elasticity;
};

struct Body
{
	DirectX::XMFLOAT3 position;
	float elasticity;
	DirectX::XMFLOAT3 linVelocity;
	float massInv;
	DirectX::XMFLOAT4 rotation;
	DirectX::XMFLOAT3 angVelocity;

	Shape shape;

	void UpdateBody(
		float dt);

	void ApplyLinearImpulse(
		DirectX::XMFLOAT3* Impulse);

	void GetCenterOfMass(
		DirectX::XMFLOAT3* centerOfMass);
};

