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
	DirectX::XMFLOAT4 rotation;
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
		const DirectX::XMFLOAT3* Impulse);

	void ApplyAngularImpulse(
		const DirectX::XMFLOAT3* Impulse);

	void ApplyImpulse(
		const DirectX::XMFLOAT3* contactPoint,
		const DirectX::XMFLOAT3* Impulse);

	void GetCenterOfMassWorldSpace(
		DirectX::XMFLOAT3* centerOfMass) const;

	void GetPointInLocalSpace(
		const DirectX::XMFLOAT3* point,
		DirectX::XMFLOAT3* localSpacePoint) const;

	void GetInverseInertiaTensorWorldSpace(
		DirectX::XMFLOAT4X4* tensor) const;
};

