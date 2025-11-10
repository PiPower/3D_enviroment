#pragma once
#include "window.hpp"
#include <DirectXMath.h>
#include "Renderer//Renderer.hpp"
struct  CameraOrientation
{
	DirectX::XMFLOAT3 eye;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 lookDir;
	CameraOrientation(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 up, DirectX::XMFLOAT3 lookDir) :
		eye(eye), up(up), lookDir(lookDir) {}
};

struct Entity
{
	ObjectTransform transform;
};

struct Composer
{
	Composer(
		Renderer* renderer);

	void RenderScene();

	void UpdateCamera();

	void GenerateObjects();

	void UpdateObjects();

	void ProcessUserInput(
		Window* window,
		float dt);
public:
	Renderer* renderer;
	float cameraAngleX;
	float cameraAngleY;
	uint64_t boxCollection;
	uint64_t uboPool;
	// ----- Entity related -----
	std::vector<Entity> entities;
	std::vector<RenderItem> renderEntities;
	// ----- camera related -----
	uint64_t cameraUbo;
	Camera cameraBuffer;
	CameraOrientation camOrientation;

};

