#include "Body.hpp"

using namespace DirectX;

void Body::UpdateBody(
	float dt)
{
	XMVECTOR dr = XMLoadFloat3(&linVelocity) * dt;
	XMVECTOR v_newPosition = dr + XMLoadFloat3(&position);
	XMStoreFloat3(&position, v_newPosition);

	XMFLOAT3 CoM;
	GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_posToCoM = v_newPosition - XMLoadFloat3(&CoM);

	XMFLOAT4X4 partialInertiaTensor;
	shape.getPartialInertiaTensor(&shape, &partialInertiaTensor);

	XMMATRIX v_rotationMatrix =  XMMatrixRotationQuaternion(XMQuaternionNormalize(XMLoadFloat4(&rotation)));
	
	XMMATRIX v_partialInertiaTensorWorld = XMMatrixTranspose(v_rotationMatrix) *
										   XMLoadFloat4x4(&partialInertiaTensor) *
										   v_rotationMatrix;


	XMVECTOR v_angVelocity = XMLoadFloat3(&angVelocity);
	auto l = XMVector3Cross(XMVector3Transform(v_angVelocity, v_partialInertiaTensorWorld), v_angVelocity);
	XMVECTOR v_alpha = XMVector3Transform(
		XMVector3Cross(XMVector3Transform(v_angVelocity, v_partialInertiaTensorWorld), v_angVelocity ),
		XMMatrixInverse(nullptr, v_partialInertiaTensorWorld));
	
	v_angVelocity = v_angVelocity + v_alpha * dt;
	XMStoreFloat3(&angVelocity, v_angVelocity);

	XMVECTOR v_dOmega = v_angVelocity * dt;
	float angle;
	XMStoreFloat(&angle, XMVector3Length(v_dOmega));
	XMVECTOR v_dRotation = XMVector3Normalize(v_dOmega);
	v_dRotation = XMVectorSetW(v_dRotation * sinf(angle/2.0f), cosf(angle / 2.0f));

	XMVECTOR v_rotation = XMLoadFloat4(&rotation);
	v_rotation = XMQuaternionMultiply(v_dRotation, v_rotation);
	v_rotation = XMQuaternionNormalize(v_rotation);
	XMStoreFloat4(&rotation, v_rotation);

	XMStoreFloat3(&position, XMLoadFloat3(&CoM) + XMVector3Transform(v_posToCoM, XMMatrixTranspose(XMMatrixRotationQuaternion(v_dRotation))));

}

void Body::ApplyLinearImpulse(
	const XMFLOAT3* Impulse)
{
	if (massInv == 0.0f)
	{
		return;
	}

	linVelocity.x += Impulse->x * massInv;
	linVelocity.y += Impulse->y * massInv;
	linVelocity.z += Impulse->z * massInv;
}

void Body::ApplyAngularImpulse(
	const DirectX::XMFLOAT3* Impulse)
{
	if (massInv == 0)
	{
		return;
	}

	XMFLOAT4X4 invInertiaTensor;
	XMVECTOR vImpulse = XMLoadFloat3(Impulse);
	shape.getInverseInertiaTensorWorldSpace(&shape, massInv, &rotation, &invInertiaTensor);
	auto p = XMVector3Transform(vImpulse, XMLoadFloat4x4(&invInertiaTensor));
	XMStoreFloat3(&angVelocity,
		XMLoadFloat3(&angVelocity) + XMVector3Transform(vImpulse, XMLoadFloat4x4(&invInertiaTensor)) );

	constexpr float maxAngularVelocity = 30.0f;
	float len;
	XMStoreFloat(&len, XMVector3LengthSq(XMLoadFloat3(&angVelocity)));
	if (len > maxAngularVelocity * maxAngularVelocity)
	{
		XMVECTOR vAngVelocity = XMLoadFloat3(&angVelocity);
		vAngVelocity = XMVector3Normalize(vAngVelocity);
		vAngVelocity = vAngVelocity * maxAngularVelocity;
		XMStoreFloat3(&angVelocity, vAngVelocity);
	}
}

void Body::ApplyImpulse(
	const DirectX::XMFLOAT3* contactPoint,
	const DirectX::XMFLOAT3* Impulse)
{
	ApplyLinearImpulse(Impulse);

	XMFLOAT3 CoM;
	GetCenterOfMassWorldSpace(& CoM);

	XMVECTOR r = XMLoadFloat3(contactPoint) - XMLoadFloat3(&CoM);
	XMFLOAT3 dL;
	XMStoreFloat3(&dL, -1.0f * XMVector3Cross(r, XMLoadFloat3(Impulse)));
	ApplyAngularImpulse(&dL);
}

void Body::GetCenterOfMassWorldSpace(
	DirectX::XMFLOAT3* centerOfMass) const
{
	shape.getCenterOfMass(&shape, centerOfMass);
	XMVECTOR Q = XMLoadFloat4(&rotation);
	XMVECTOR Q_inv = XMQuaternionInverse(Q);
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR CoM = XMLoadFloat3(centerOfMass);

	XMStoreFloat3(centerOfMass, pos + XMQuaternionMultiply(XMQuaternionMultiply(Q, CoM), Q_inv));
}

void Body::GetPointInLocalSpace(
	const DirectX::XMFLOAT3* point,
	DirectX::XMFLOAT3* localSpacePoint) const
{
	XMFLOAT3 CoM;
	GetCenterOfMassWorldSpace(&CoM);
	
	XMMATRIX v_invRotiation = XMMatrixRotationQuaternion(XMQuaternionInverse(XMLoadFloat4(&rotation)));
	XMVECTOR v_point = XMLoadFloat3(point) - XMLoadFloat3(&CoM);
	XMVECTOR v_localPoint = XMVector3Transform(v_point, XMMatrixTranspose(v_invRotiation));
	XMStoreFloat3(localSpacePoint, v_localPoint);
}

void Body::GetInverseInertiaTensorWorldSpace(
	DirectX::XMFLOAT4X4* tensor) const
{
	shape.getInverseInertiaTensorWorldSpace(&shape, massInv, &rotation, tensor);
}

BoundingBox Body::getBoundingBox() const
{
	return shape.getBoundingBox(&shape, &position, &rotation);
}
