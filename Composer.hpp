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
		PhysicsEnigne* physicsEngine);

	void RenderScene();

	void UpdateCamera();

	void GenerateObjects();

	void UpdateObjects(
		float dt);

	void ProcessUserInput(
		Window* window,
		float dt);
public:
	Renderer* renderer;
	PhysicsEnigne* physicsEngine;
	float cameraAngleX;
	float cameraAngleY;
	uint64_t boxCollection;
	uint64_t uboPool;
	 
	// ----- Entity related -----
	std::vector<uint64_t> physicsEntities;
	std::vector<ObjectUbo> physicsEntitiesTrsfm;
	std::vector<RenderItem> renderEntities;
	// ----- camera related -----
	uint64_t cameraUbo;
	Camera cameraBuffer;
	CameraOrientation camOrientation;

};

