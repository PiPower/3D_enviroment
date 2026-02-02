#include "Composer.hpp"
#include "Renderer/CommonShapes.hpp"
#include <random>
using namespace DirectX;
using namespace std;

static constexpr XMFLOAT3 gravityForce = { 0, -15, 0 };
static constexpr XMFLOAT3 eyeInitial = { -10.0f, 9.0f, -25.0f };
static constexpr XMFLOAT3 lookDirInitial = { 0.0f, 0.0f, 1.0f };
static constexpr XMFLOAT3 upInitial = { 0.0f, 1.0f, 0.0f };

struct Texel
{
    char r, g, b, a;
};

struct PointProjection
{
    float dist;
    XMFLOAT3 ptOnSurfA;
    XMFLOAT3 ptOnSurfB;
    size_t idBodyA;
    size_t idBodyB;
};

Composer::Composer(
    Renderer* renderer,
    PhysicsEnigne* physicsEngine)
    :
    cameraAngleX(0), cameraAngleY(0), camOrientation(eyeInitial, upInitial, lookDirInitial),
    renderer(renderer), physicsEngine(physicsEngine), calculatePhysics(true), frameMode(true),
    characterVelocity({ 0, 0, 0 }), constForce(gravityForce), dragCoeff({ 0.98f, 1.0f,  0.98f }),
    orientationDir({ 1, 0, 0 }), forwardDir({ 1, 0, 0 }), upDir({ 0,1,0 }), rightDir({ 0,0,-1 }),
    characterVelocityCoeff({ 70000, 70000, 70000 }), freeFall(true), scalesVel({1.0f, 1.0f, 1.0f})
{
    lights = { Light{{1, 1, 1, 1.0f}, {0.0f, 100.0f, 0.0f, 0.2f} } };
    vector<TextureDim> dims(10, TextureDim{ 300, 300});

    renderer->CreateGraphicsPipeline(0, {}, &shadowmapPipelineId, true);
    renderer->CreateGraphicsPipeline(lights.size(), dims ,&pipelineId);

    vector<GeometryEntry> boxGeo({ GeometryType::Box });
    renderer->CreateMeshCollection(boxGeo, &boxCollection);
    renderer->CreateUboPool(sizeof(Camera) + lights.size() * sizeof(Light), sizeof(ObjectUbo), 300'000, &uboPool);
    renderer->AllocateUboResource(uboPool, UBO_GLOBAL_RESOURCE_TYPE, &shadowmapGlobalUbo);
    renderer->AllocateUboResource(uboPool, UBO_GLOBAL_RESOURCE_TYPE, &globalUbo);
    renderer->BindUboPoolToPipeline(shadowmapPipelineId, uboPool, shadowmapGlobalUbo);
    renderer->BindUboPoolToPipeline(pipelineId, uboPool, globalUbo);

    globalUboBuffer = new char[sizeof(Camera) + lights.size() * sizeof(Light)];
    shadowmapUboBuffer = new char[sizeof(Camera) + lights.size() * sizeof(Light)];

    UpdateShadowmapGlobalBuffer();
    UpdateGlobalBuffer();
    GenerateObjects();
    UpdateTextures(dims, { 1000,  1000 });
}

void Composer::RenderScene()
{
    UpdateCamera();

    renderer->BeginRendering();
    renderer->Render(boxCollection, pipelineId, renderEntities);
    renderer->Present();
}

void Composer::AddWalkableCuboid(
    uint64_t bodyId, 
    uint8_t faceId)
{
    walkableCuboids.push_back({ bodyId, faceId });
}

void Composer::UpdateTextures(
    const std::vector<TextureDim>& dims,
    TextureDim skyboxDims)
{
    Texel* texData = new Texel[1000 * 1000];

    Texel sourceTexels[7] = { {255, 255, 255, 255},
                              {12, 56, 230, 255},
                              {120, 120, 50, 255},
                              {2, 150, 2, 255},
                              {120, 5, 5, 255},
                              {40, 40, 40, 255},
                              {90, 233, 233, 255} };

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < dims[i].height * dims[i].width; j++)
        {
            texData[j].a = sourceTexels[i].a;
            texData[j].r = sourceTexels[i].r;
            texData[j].g = sourceTexels[i].g;
            texData[j].b = sourceTexels[i].b;
        }
        renderer->UploadTexture(pipelineId, i, (char*)texData);
    }


    for (int j = 0; j < skyboxDims.width * skyboxDims.height; j++)
    {
        texData[j].a = sourceTexels[6].a;
        texData[j].r = sourceTexels[6].r;
        texData[j].g = sourceTexels[6].g;
        texData[j].b = sourceTexels[6].b;
    }
    
    for (int y = 0; y < skyboxDims.height; y++)
    {
        for (int x = 0; x < skyboxDims.width; x++)
        {
            float x0 = ((float)x / (float)skyboxDims.width) * 3 - 2.2f;
            float y0 = ((float)y / (float)skyboxDims.height) * 3 - 1.4f;
            float z_x = 0;
            float z_y = 0;
            constexpr uint16_t max_iteration = 300;
            uint16_t i = 0;
            while(z_x * z_x + z_y * z_y < 4.0f &&  i < max_iteration)
            {
                float xtemp = z_x * z_x - z_y * z_y + x0;
                z_y = 2 * z_x * z_y + y0;
                z_x = xtemp;
                i++;
            }


            texData[skyboxDims.width * y + x].a = 255;
            texData[skyboxDims.width * y + x].r = ((float)i / (float)max_iteration) * 255;
            texData[skyboxDims.width * y + x].g = sinf(((float)i / (float)max_iteration) * 2 * 3.14) * 255;
            texData[skyboxDims.width * y + x].b = ((float)i / (float)max_iteration) * 255;
        }
    }



     vector<const char*> textures{ (char*)texData, (char*)texData, (char*)texData, (char*)texData, (char*)texData, (char*)texData };
     renderer->UpdateSkyboxData(textures, uboPool, globalUbo);
    


    delete[] texData;
}

bool Composer::CheckIfObjIsOnWalkableCuboidSurface(
    const DirectX::XMFLOAT3& ptOnCuboid,
    size_t cuboidIdx)
{
    XMFLOAT3 faceNormal;
    WalkableCuboid& cuboidDex = walkableCuboids[cuboidIdx];
    Body* cuboid = physicsEngine->GetBody(cuboidDex.bodyId);
    cuboid->GetLocalSpaceFaceNormalFromPoint(&ptOnCuboid, &faceNormal);

    if (faceNormal.x == 1.0f && (cuboidDex.faceIds & w_right) > 0) { return true; };
    if (faceNormal.x == -1.0f && (cuboidDex.faceIds & w_left) > 0) { return true; };
    if (faceNormal.y == 1.0f && (cuboidDex.faceIds & w_top) > 0) { return true; };
    if (faceNormal.y == -1.0f && (cuboidDex.faceIds & w_bottom) > 0) { return true; };
    if (faceNormal.z == 1.0f && (cuboidDex.faceIds & w_back) > 0) { return true; };
    if (faceNormal.z == -1.0f && (cuboidDex.faceIds & w_front) > 0) { return true; };
    return false;
}

void Composer::UpdateCamera()
{
    XMVECTOR eyePos = XMLoadFloat3(&camOrientation.eye);
    XMVECTOR lookAt = XMLoadFloat3(&camOrientation.lookDir);
    XMVECTOR up = XMLoadFloat3(&camOrientation.up);
    XMFLOAT4X4 view;
    XMStoreFloat4x4(&view, XMMatrixLookToLH(eyePos, lookAt, up));
    memcpy(globalUboBuffer, &view, sizeof(XMFLOAT4X4));

    renderer->UpdateUboMemory(uboPool, globalUbo, globalUboBuffer);
}

void Composer::GenerateObjects()
{
    // character
    {
        BodyProperties bodyProps;
        bodyProps.position = { -3, 1, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 1.0f/40.f;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 0.0f;
        bodyProps.friction = 0.0f;
        XMFLOAT3 scales = { 0.5f, 0.5f, 0.5f};
        XMUINT4 objInfo = {0, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, false, { 0, 0, 0 });
        characterId = physicsEntities.back();
    }
    
    // collider
    {
        BodyProperties bodyProps;
        bodyProps.position = { 10, 2, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 1.0f / 20.f;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 0.5f;
        bodyProps.friction = 1.0f;
        XMFLOAT3 scales = { 1.0f, 1.0f, 1.0f };
        XMUINT4 objInfo = { 1, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
    }

    // floor
    {
        BodyProperties bodyProps;
        bodyProps.position = { 0, -2, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 35, 1, 35 };
        XMUINT4 objInfo = { 2, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
        AddWalkableCuboid(physicsEntities.back(), w_top);
    }
    // left wall
    {
        BodyProperties bodyProps;
        bodyProps.position = { -35, 0, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 1, 20, 35 };
        XMUINT4 objInfo = { 3, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
    }

    // right wall
    {
        BodyProperties bodyProps;
        bodyProps.position = { 35, 0, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 1, 20, 35 };
        XMUINT4 objInfo = { 3, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
    }

    // front wall
    {
        BodyProperties bodyProps;
        bodyProps.position = { 0, 0, -35 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 35, 20, 1 };
        XMUINT4 objInfo = { 3, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
    }

    // back wall
    {
        BodyProperties bodyProps;
        bodyProps.position = { 0, 0, 35 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 35, 20, 1 };
        XMUINT4 objInfo = { 3, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
    }


    // ramp
    {
        BodyProperties bodyProps;
        bodyProps.position = { 0, 0, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        float angle = -3.14f / 6.0f;
        bodyProps.rotation = { 0, 0, 1.0f * sinf(angle/2.0f), cosf(angle/ 2.0f)};
        bodyProps.elasticity = 0.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 5, 1, 1 };
        XMUINT4 objInfo = { 4, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
        AddWalkableCuboid(physicsEntities.back(), w_top);
    }

    // balcony
    {
        BodyProperties bodyProps;
        bodyProps.position = { 8.85, 2.374, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 1.0f;
        bodyProps.friction = 0.01f;
        XMFLOAT3 scales = { 5, 1, 5 };
        XMUINT4 objInfo = { 5, 0, 0 ,0 };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, objInfo, bounds, true);
        AddWalkableCuboid(physicsEntities.back(), w_top);
    }

}

void Composer::UpdateObjects(
    float dt)
{
    if (calculatePhysics)
    {
        XMFLOAT3 velocity;
        XMStoreFloat3(&velocity,
           XMLoadFloat3(&scalesVel) * 
           ( characterVelocity.z * XMLoadFloat3(&forwardDir) + characterVelocity.x * XMLoadFloat3(&rightDir)));


        physicsEngine->AddForce(characterId, X_COMPONENT | Y_COMPONENT | Z_COMPONENT, constForce);
        physicsEngine->AddForce(characterId, X_COMPONENT | Y_COMPONENT | Z_COMPONENT, velocity);
        physicsEngine->UpdateBodies(dt);

  

        physicsEngine->GetLinearVelocity(characterId, &velocity);
        velocity.x *= dragCoeff.x;
        velocity.y *= dragCoeff.y;
        velocity.z *= dragCoeff.z;
        physicsEngine->SetLinearVelocity(characterId, X_COMPONENT | Y_COMPONENT | Z_COMPONENT, velocity);

        if (frameMode)
        {
            calculatePhysics = false;
        }
    }

    for (size_t i = 0; i < physicsEntitiesTrsfm.size(); i++)
    {
        physicsEngine->GetTransformMatrixForBody(physicsEntities[i], &physicsEntitiesTrsfm[i].transform);
        renderer->UpdateUboMemory(uboPool, renderEntities[i].transformUboId, (char*) &physicsEntitiesTrsfm[i].transform);
    }

    UpdateMovementVectors();
    characterVelocity = { 0, 0, 0 };
    return;
}

void Composer::ProcessUserInput(
    Window* window,
    float dt)
{

    XMVECTOR eyeVec = XMLoadFloat3(&camOrientation.eye);
    XMVECTOR upVec = XMLoadFloat3(&camOrientation.up);
    XMVECTOR lookDirVec = XMLoadFloat3(&camOrientation.lookDir);

    XMVECTOR upInitialVec = XMLoadFloat3(&upInitial);


    Window::KeyEvent event = window->ReadKeyEvent();
    while (event.Type != Window::KeyEvent::Event::Invalid)
    {
        if (event.Code == 'T' && event.Type == Window::KeyEvent::Event::Press)
        {
            calculatePhysics = !calculatePhysics;
        }
        else if (event.Code == 'Y' && event.Type == Window::KeyEvent::Event::Press)
        {
            frameMode = !frameMode;
        }
        event = window->ReadKeyEvent();
    }
    
    if (window->IsKeyPressed('W')) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(lookDirVec, dt * 10.0f)); }
    if (window->IsKeyPressed('S')) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(lookDirVec, dt * 10.0f)); }
    if (window->IsKeyPressed(VK_SPACE)) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(upInitialVec, dt * 10.0f)); }
    if (window->IsKeyPressed(VK_CONTROL)) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(upInitialVec, dt * 10.0f)); }
    if (window->IsKeyPressed('D')) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(XMVector3Cross(upVec, lookDirVec), dt * 10.0f)); }
    if (window->IsKeyPressed('A')) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(XMVector3Cross(upVec, lookDirVec), dt * 10.0f)); }

    if (window->IsKeyPressed(VK_UP))
    {
        characterVelocity.z += characterVelocityCoeff.z * dt;
    }
    if (window->IsKeyPressed(VK_DOWN))
    {
        characterVelocity.z -= characterVelocityCoeff.z * dt;
    }
    if (window->IsKeyPressed(VK_LEFT))
    {
        characterVelocity.x -= characterVelocityCoeff.x * dt;
    }
    if (window->IsKeyPressed(VK_RIGHT))
    {
        characterVelocity.x += characterVelocityCoeff.x * dt;
    }
    


    if (window->IsLeftPressed())
    {
        static float angleX = 0.0f, angleY = 0.0f;
        if (window->GetMouseDeltaX() < 0)
        {
            angleY += dt * 5.0f;
        }
        else if (window->GetMouseDeltaX() > 0)
        {
            angleY += -dt * 5.0f;
        }

        if (window->GetMouseDeltaY() < 0)
        {
            angleX += -dt * 5.0f;
        }
        else if (window->GetMouseDeltaY() > 0)
        {
            angleX += dt * 5.0f;
        }

        /*
        XMMATRIX azimRotation = XMMatrixRotationY(angleY);
        lookDirVec = XMVector3Transform(XMLoadFloat3(&lookDirInitial), azimRotation);
        upVec = XMVector3Transform(XMLoadFloat3(&upInitial), azimRotation);

        XMVECTOR rotAxis = XMVector3Cross(upVec, lookDirVec);
        XMMATRIX elevRotation = XMMatrixRotationAxis(rotAxis, angleX);
        lookDirVec = XMVector3Transform(lookDirVec, elevRotation);
        upVec = XMVector3Transform(upVec, elevRotation);*/

        // IMPORTANT: Order MATTERS !!!!
        angleX = std::clamp(angleX, -3.14f / 2.0f, 3.14f / 2.0f);

        XMMATRIX rotMatrix = XMMatrixRotationX(angleX) * XMMatrixRotationY(angleY);

        lookDirVec = XMVector3Transform(XMLoadFloat3(&lookDirInitial), rotMatrix);
        upVec = XMVector3Transform(XMLoadFloat3(&upInitial), rotMatrix);


    }

    XMStoreFloat3(&camOrientation.eye, eyeVec);
    XMStoreFloat3(&camOrientation.up, upVec);
    XMStoreFloat3(&camOrientation.lookDir, lookDirVec);
}

void Composer::UpdateGlobalBuffer()
{
    memcpy(globalUboBuffer + sizeof(Camera), lights.data(), lights.size() * sizeof(Light));
    
    XMFLOAT4X4 proj;
    XMStoreFloat4x4(&proj, XMMatrixPerspectiveFovLH(3.14f / 4.0f, 1600.0f / 900.0f, 0.1f, 128.0f));
    proj._22 *= -1.0f;// vulkan has -1.0f on top and 1.0f on bottom

    memcpy(globalUboBuffer + sizeof(XMFLOAT4X4), &proj, sizeof(XMFLOAT4X4));
}

void Composer::UpdateShadowmapGlobalBuffer()
{
    XMFLOAT4X4 proj;
    XMStoreFloat4x4(&proj, XMMatrixOrthographicLH(3.14f / 4.0f, 1600.0f / 900.0f, 0.1f, 128.0f));
    proj._22 *= -1.0f;// vulkan has -1.0f on top and 1.0f on bottom
    memcpy(globalUboBuffer + sizeof(XMFLOAT4X4), &proj, sizeof(XMFLOAT4X4));

    XMVECTOR lightPos = XMLoadFloat4(&lights[0].pos);
    XMVECTOR lightDir = XMVectorSet(0, 1, 0 ,0 );
    XMVECTOR lightUp = XMVectorSet(1, 0, 0, 0);
    XMFLOAT4X4 view;
    XMStoreFloat4x4(&view, XMMatrixLookToLH(lightPos, lightDir, lightUp));

    memcpy(shadowmapUboBuffer, &view, sizeof(XMFLOAT4X4));
    renderer->UpdateUboMemory(uboPool, shadowmapGlobalUbo, globalUboBuffer);
}

void Composer::AddBody(
    const ShapeType& type,
    const BodyProperties& props,
    const DirectX::XMFLOAT3& scales,
    const DirectX::XMUINT4& objInfo,
    const LinearVelocityBounds& vBounds,
    bool allowAngularImpulse,
    const DirectX::XMFLOAT3& constForce)
{
    uint64_t uboId;
    renderer->AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &uboId);
    renderEntities.push_back({ 0, 0, uboId, 0 });

    size_t entitySize = physicsEntities.size();
    physicsEntities.push_back(0);
    physicsEntitiesTrsfm.push_back({});

    bool isDynamic = props.massInv != 0.0f;
    physicsEngine->AddBody(props, type, scales, isDynamic, &physicsEntities[entitySize], allowAngularImpulse, vBounds, constForce);
    physicsEngine->GetTransformMatrixForBody(physicsEntities[entitySize], &physicsEntitiesTrsfm[entitySize].transform);

    physicsEntitiesTrsfm[entitySize].objInfo[0] = objInfo.x;
    physicsEntitiesTrsfm[entitySize].objInfo[1] = objInfo.y;
    physicsEntitiesTrsfm[entitySize].objInfo[2] = objInfo.z;
    physicsEntitiesTrsfm[entitySize].objInfo[3] = objInfo.w;

}

void Composer::UpdateMovementVectors()
{
    PointProjection projection{ 1e10 };

    size_t id = 0;
    for (size_t j = 0; j < walkableCuboids.size(); j++)
    {
        float dist;
        XMFLOAT3 ptOnCharacter, ptOnSurface;
        physicsEngine->GetDistanceBetweenBodies(characterId, walkableCuboids[j].bodyId, &ptOnCharacter, &ptOnSurface, &dist);
        if (dist < projection.dist)
        {
            projection.dist = dist;
            projection.ptOnSurfA = ptOnCharacter;
            projection.ptOnSurfB = ptOnSurface;
            projection.idBodyA = characterId;
            projection.idBodyB = walkableCuboids[j].bodyId;
            id = j;
        }
    }

    if (!CheckIfObjIsOnWalkableCuboidSurface(projection.ptOnSurfB, id))
    {
        return;
    }

    Body* surface = physicsEngine->GetBody(projection.idBodyB);
    surface->GetWorldSpaceFaceNormalFromPoint(&projection.ptOnSurfB, &upDir);

    XMVECTOR v_upDir = XMLoadFloat3(&upDir);
    XMVECTOR v_orientation = XMLoadFloat3(&orientationDir);
    XMVECTOR v_projection = -v_upDir * XMVector3Dot(v_orientation, -v_upDir);
    XMVECTOR v_forwardDir = XMVector3Normalize(v_orientation - v_projection);
    XMStoreFloat3(&forwardDir, v_forwardDir);
    XMStoreFloat3(&rightDir, XMVector3Normalize(XMVector3Cross(v_forwardDir, -v_upDir)));
    //lastSurface = walkableCuboids[j].bodyId;

    constForce = { 0, 0, 0 };
    scalesVel = { 1, 1, 1 };
    if (characterVelocity.z == 0.0f && characterVelocity.x == 0.0f && characterVelocity.y == 0.0f)
    {
        //dragCoeff = { 0.7f, 0.7f, 0.7f };
    }
    else
    {
        dragCoeff = { 0.99f, 0.99f, 0.99f };
    }

    if (projection.dist > 0.1f)
    {
        constForce = { 0, -30, 0 };
        if (projection.dist > 0.2f)
        {
            dragCoeff.y = 1.0f;
        }
        if (projection.dist > 0.5f)
        {
            dragCoeff = { 0.7f, 1.0f,  0.7f };
            scalesVel = { 0, 0, 0 };
        }
    }

}
