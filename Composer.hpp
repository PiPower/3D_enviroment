#pragma once
#include "window.hpp"
#include <DirectXMath.h>
#include "Renderer//Renderer.hpp"
#include "Physics/PhysicsEnigne.h"
#include <set>
struct  CameraOrientation
{
	DirectX::XMFLOAT3 eye;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 lookDir;
	CameraOrientation(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 up, DirectX::XMFLOAT3 lookDir) :
		eye(eye), up(up), lookDir(lookDir) {}
};


struct Composer
{
	Composer(
		Renderer* renderer,
		PhysicsEnigne* physicsEngine,
		const std::vector<Light>& lights);

	void RenderScene();

	void UpdateCamera();

	void GenerateObjects();

	void UpdateObjects(
		float dt);

	void ProcessUserInput(
		Window* window,
		float dt);

	void AddBody( const ShapeType& type, 
		const BodyProperties& props,
		const DirectX::XMFLOAT3& scales, 
		const DirectX::XMFLOAT4& color,
		const LinearVelocityBounds& vBounds,
		bool allowAngularImpulse,
		const DirectX::XMFLOAT3& constForce = { 0, -15, 0 });
public:
	Renderer* renderer;
	PhysicsEnigne* physicsEngine;
	float cameraAngleX;
	float cameraAngleY;
	uint64_t boxCollection;
	uint64_t uboPool;
	bool calculatePhysics;
	bool frameMode;
	// ----- character management ----- 
	DirectX::XMFLOAT3 characterVelocity;
	DirectX::XMFLOAT3 characterVelocityCoeff;
	DirectX::XMFLOAT3 orientationDir;
	DirectX::XMFLOAT3 forwardDir;
	DirectX::XMFLOAT3 rightDir;
	DirectX::XMFLOAT3 upDir;
	DirectX::XMFLOAT3 constForce;
	std::vector<uint64_t> walkableSurface;
	uint64_t lastSurface;
	uint64_t characterId;
	float dragCoeff;
	bool freeFall;
	// ----- Entity related -----
	std::vector<uint64_t> physicsEntities;
	std::vector<ObjectUbo> physicsEntitiesTrsfm;
	std::vector<RenderItem> renderEntities;
	// ----- camera related -----
	uint64_t globalUbo;
	CameraOrientation camOrientation;
	std::vector<Light> lights;
	char* globalUboBuffer;
};

