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

constexpr uint8_t w_top = 0x1;
constexpr uint8_t w_bottom = 0x1 << 1;
constexpr uint8_t w_front = 0x1 << 2;
constexpr uint8_t w_back = 0x1 << 3;
constexpr uint8_t w_left = 0x1 << 4;
constexpr uint8_t w_right = 0x1 << 5;

struct WalkableCuboid
{
	uint64_t bodyId;
	uint8_t faceIds;
};

struct Composer
{
	Composer(
		Renderer* renderer,
		PhysicsEnigne* physicsEngine);

	void RenderScene();

	void AddWalkableCuboid(
		uint64_t bodyId, 
		uint8_t faceId);

	void UpdateTextures(
		const std::vector<TextureDim>& dims,
		TextureDim skyboxDims = { 0, 0 });

	bool CheckIfObjIsOnWalkableCuboidSurface(
		const DirectX::XMFLOAT3& ptOnCuboid,
		size_t cuboidIdx);

	void UpdateCamera();

	void GenerateObjects();

	void UpdateObjects(
		float dt);

	void ProcessUserInput(
		Window* window,
		float dt);

	void UpdateGlobalBuffer();
	
	void UpdateShadowmapGlobalBuffer();

	void AddBody( const ShapeType& type, 
		const BodyProperties& props,
		const DirectX::XMFLOAT3& scales, 
		const DirectX::XMUINT4& objInfo,
		const LinearVelocityBounds& vBounds,
		bool allowAngularImpulse,
		const DirectX::XMFLOAT3& constForce = { 0, -15, 0 });

	void UpdateMovementVectors();

	void GetLightViewMatrix(
		DirectX::XMFLOAT4X4* lightViewMatrix);
public:
	Renderer* renderer;
	PhysicsEnigne* physicsEngine;
	float cameraAngleX;
	float cameraAngleY;
	uint64_t boxCollection;
	uint64_t uboPool;
	bool calculatePhysics;
	bool frameMode;
	uint64_t pipelineId;
	uint64_t shadowmapPipelineId;
	// ----- character management ----- 
	DirectX::XMFLOAT3 scalesVel;
	DirectX::XMFLOAT3 characterVelocity;
	DirectX::XMFLOAT3 characterVelocityCoeff;
	DirectX::XMFLOAT3 orientationDir;
	DirectX::XMFLOAT3 forwardDir;
	DirectX::XMFLOAT3 rightDir;
	DirectX::XMFLOAT3 upDir;
	DirectX::XMFLOAT3 constForce;
	DirectX::XMFLOAT3 dragCoeff;
	std::vector<WalkableCuboid> walkableCuboids;
	uint64_t lastSurface;
	uint64_t characterId;
	bool freeFall;
	// ----- Entity related -----
	std::vector<uint64_t> physicsEntities;
	std::vector<ObjectUbo> physicsEntitiesTrsfm;
	std::vector<RenderItem> renderEntities;
	// ----- camera related -----
	uint64_t globalUbo;
	uint64_t shadowmapGlobalUbo;
	CameraOrientation camOrientation;
	std::vector<Light> lights;
	char* globalUboBuffer;
	char* shadowmapUboBuffer;
};

