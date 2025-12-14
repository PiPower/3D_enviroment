#include "Body.hpp"

using namespace DirectX;

void Body::UpdateBody(
	float dt)
{
	XMVECTOR v_newPosition = XMLoadFloat3(&linVelocity) * dt + XMLoadFloat3(&position);
	XMStoreFloat3(&position, v_newPosition);

	XMFLOAT3 CoM;
	GetCenterOfMassWorldSpace(&CoM);
	XMVECTOR v_posToCoM = v_newPosition - XMLoadFloat3(&CoM);

	XMFLOAT4X4 partialInertiaTensor;
	shape.getPartialInertiaTensor(&shape, &partialInertiaTensor);

	XMMATRIX v_rotationMatrix = XMMatrixRotationQuaternion(XMQuaternionNormalize(XMLoadFloat4(&rotation)));
	
	XMMATRIX v_partialInertiaTensorWorld = XMMatrixMultiply(
		XMMatrixMultiply(v_rotationMatrix, XMLoadFloat4x4(&partialInertiaTensor)), 
		XMMatrixTranspose(v_rotationMatrix));


	XMVECTOR v_angVelocity = XMLoadFloat3(&angVelocity);

	XMVECTOR v_alpha = XMVector3Transform(
		XMVector3Cross(v_angVelocity, XMVector3Transform(v_angVelocity, v_partialInertiaTensorWorld) ),
		XMMatrixInverse(nullptr, v_partialInertiaTensorWorld));
	
	v_angVelocity = v_angVelocity + v_alpha * dt;
	XMStoreFloat3(&angVelocity, v_angVelocity);

	XMVECTOR v_dOmega = v_angVelocity * dt;
	float angle;
	XMStoreFloat(&angle, XMVector3Length(v_dOmega));
	XMVECTOR v_dRotation = v_dOmega;
	v_dRotation = XMVectorSetW(v_dRotation, angle);

	XMVECTOR v_rotation = XMLoadFloat4(&rotation);
	v_rotation = XMQuaternionMultiply(v_dRotation, v_rotation);
	v_rotation = XMQuaternionNormalize(v_rotation);
	XMStoreFloat4(&rotation, v_rotation);

	XMStoreFloat3(&position, XMLoadFloat3(&CoM) + XMVector3Transform(v_posToCoM, XMMatrixRotationQuaternion(v_dRotation)));

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

	XMStoreFloat3(&angVelocity,
		XMLoadFloat3(&angVelocity) + XMVector3Transform(vImpulse, XMLoadFloat4x4(&invInertiaTensor)) );

	constexpr float maxAngularVelocity = 30.0f;
	float len;
	XMStoreFloat(&len, XMVector3LengthSq(XMLoadFloat3(&angVelocity)));
	if (len > maxAngularVelocity * maxAngularVelocity)
	{
		XMVECTOR vAngVelocity = XMLoadFloat3(&angVelocity);
		XMVector3Normalize(vAngVelocity);
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
	XMStoreFloat3(&dL, XMVector3Cross(r, XMLoadFloat3(Impulse)));
	ApplyAngularImpulse(&dL);
}

void Body::GetCenterOfMassWorldSpace(
	XMFLOAT3* centerOfMass)
{
	shape.getCenterOfMass(&shape, centerOfMass);
	XMVECTOR Q = XMLoadFloat4(&rotation);
	XMVECTOR Q_inv = XMQuaternionInverse(Q);
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR CoM = XMLoadFloat3(centerOfMass);

	XMStoreFloat3(centerOfMass, pos + XMQuaternionMultiply(XMQuaternionMultiply(Q, CoM), Q_inv));
}
