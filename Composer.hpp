#pragma once
#include "window.hpp"
#include <DirectXMath.h>
#include "Renderer//Renderer.hpp"
#include "Physics/PhysicsEnigne.h"

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
		bool allowAngularImpulse);
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
	DirectX::XMFLOAT4X4 surfaceRotation;
	uint64_t rampId;
	bool onTheSurface;
	float timeNotOnTheSurface;
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

