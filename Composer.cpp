#include "Composer.hpp"
#include "Renderer/CommonShapes.hpp"
#include <random>
using namespace DirectX;
using namespace std;

static constexpr XMFLOAT3 gravityForce = { 0, -15, 0 };
static constexpr XMFLOAT3 eyeInitial = { -10.0f, 9.0f, -25.0f };
static constexpr XMFLOAT3 lookDirInitial = { 0.0f, 0.0f, 1.0f };
static constexpr XMFLOAT3 upInitial = { 0.0f, 1.0f, 0.0f };
static constexpr uint64_t graphicsPipelineType = (uint64_t)PipelineTypes::Graphics;

Composer::Composer(
    Renderer* renderer,
    PhysicsEnigne* physicsEngine,
    const std::vector<Light>& lights)
    :
    cameraAngleX(0), cameraAngleY(0), camOrientation(eyeInitial, upInitial, lookDirInitial),
    renderer(renderer), physicsEngine(physicsEngine), calculatePhysics(true), frameMode(true),
    lights(lights), characterVelocity({ 0, 0, 0 }), constForce(gravityForce), dragCoeff({ 0.98f, 1.0f,  0.98f }),
    orientationDir({ 0, 0, 1 }), forwardDir({ 0, 0, 1 }), upDir({ 0,1,0 }), rightDir({ 1,0,0 }),
    characterVelocityCoeff({ 50000, 50000, 50000 }), freeFall(true)
{

    vector<GeometryEntry> boxGeo({ GeometryType::Box });
    renderer->CreateMeshCollection(boxGeo, &boxCollection);
    renderer->CreateUboPool(sizeof(Camera) + lights.size() * sizeof(Light), sizeof(ObjectUbo), 300'000, &uboPool);
    renderer->AllocateUboResource(uboPool, UBO_GLOBAL_RESOURCE_TYPE, &globalUbo);
    renderer->BindUboPoolToPipeline(graphicsPipelineType, uboPool, globalUbo);

    globalUboBuffer = new char[sizeof(Camera) + lights.size() * sizeof(Light)];
    memcpy(globalUboBuffer + sizeof(Camera), lights.data(), lights.size() * sizeof(Light));

    XMFLOAT4X4 proj;
    XMStoreFloat4x4(&proj, XMMatrixPerspectiveFovLH(3.14f / 4.0f, 1600.0f / 900.0f, 0.1f, 128.0f));
    proj._22 *= -1.0f;// vulkan has -1.0f on top and 1.0f on bottom

    memcpy(globalUboBuffer + sizeof(XMFLOAT4X4), &proj, sizeof(XMFLOAT4X4));

    GenerateObjects();
}

void Composer::RenderScene()
{
    UpdateCamera();

    renderer->BeginRendering();
    renderer->Render(boxCollection, graphicsPipelineType, renderEntities);
    renderer->Present();
}

void Composer::AddWalkableCuboid(
    uint64_t bodyId, 
    uint8_t faceId)
{
    walkableCuboids.push_back({ bodyId, faceId });
}

bool Composer::CheckIfObjIsOnWalkableCuboidSurface(
    const DirectX::XMFLOAT3& ptOnCuboid,
    size_t cuboidIdx)
{
    XMFLOAT3 faceNormal;
    WalkableCuboid& cuboidDex = walkableCuboids[cuboidIdx];
    Body* cuboid = physicsEngine->GetBody(cuboidDex.bodyId);
    cuboid->GetFaceNormalFromPoint(&ptOnCuboid, &faceNormal);

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
        XMFLOAT4 color = { 1.0f,  1.0f,  1.0f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, false, { 0, 0, 0 });
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
        XMFLOAT4 color = { 1.0f,  1.0f,  1.0f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.2f, 0.5f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
        XMFLOAT4 color = { 0.6f, 0.6f, 0.1f, 1.0f };
        LinearVelocityBounds bounds = { -1000, 1000, -1000, 1000, -1000, 1000 };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, bounds, true);
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
            characterVelocity.z * XMLoadFloat3(&forwardDir) + characterVelocity.x * XMLoadFloat3(&rightDir));


        physicsEngine->AddForce(characterId, X_COMPONENT | Y_COMPONENT | Z_COMPONENT, constForce);
        physicsEngine->AddForce(characterId, X_COMPONENT | Y_COMPONENT | Z_COMPONENT, velocity);
        
        physicsEngine->UpdateBodies(dt);

        characterVelocity = { 0, 0, 0 };

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

    size_t i;
    for (i = 0; i < physicsEngine->contactPoints.size() -1 ; i++)
    {
        for (size_t j = 0; j < walkableCuboids.size(); j++)
        {
            Body* character = physicsEngine->GetBody(characterId);
            if (physicsEngine->contactPoints[i].bodyA != character &&
                physicsEngine->contactPoints[i].bodyB != character)
            {
                continue;
            }

            Body* cuboid = physicsEngine->GetBody(walkableCuboids[j].bodyId);
            if (physicsEngine->contactPoints[i].bodyA == cuboid ||
                physicsEngine->contactPoints[i].bodyB == cuboid )
            {
                Contact *contact = &physicsEngine->contactPoints[i];
                XMFLOAT3* ptOnCuboid = physicsEngine->contactPoints[i].bodyA == cuboid ?
                                        &contact->ptOnA : &contact->ptOnB;

                if (!CheckIfObjIsOnWalkableCuboidSurface(*ptOnCuboid, j))
                {
                    continue;
                }


                upDir = contact->normal;
                XMVECTOR v_upDir = XMLoadFloat3(&upDir);
                XMVECTOR v_orientation = XMLoadFloat3(&orientationDir);
                XMVECTOR v_forwardDir = XMVector3Normalize(v_orientation - XMVector3Dot(v_orientation, v_upDir));
                XMStoreFloat3(&forwardDir, v_forwardDir);
                XMStoreFloat3(&rightDir, XMVector3Normalize(XMVector3Cross(v_forwardDir, v_upDir) ) );
                lastSurface = walkableCuboids[j].bodyId;
                constForce = { 0, 0, 0}; // character is on inclined object so disable gravity
                dragCoeff = { 0.85, 0.85, 0.85 };
                freeFall = false;
                goto end_of_loop;
            }
        }
    }

end_of_loop:

    if (!freeFall && i == physicsEngine->contactPoints.size() - 1)
    {
        
        float dist;
        XMFLOAT3 ptOnCharacter, ptOnSurface;
        physicsEngine->GetDistanceBetweenBodies(characterId, lastSurface, &ptOnCharacter, &ptOnSurface, &dist);
        if (dist > 0.01f && dist <= 0.2f)
        {
            constForce = { 0, -30, 0 };
        }
        
        if (dist > 0.2f)
        {
            constForce = { 0, -30, 0 };

            //upDir = upInitial;
            //forwardDir = orientationDir;
            //XMStoreFloat3(&rightDir, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&orientationDir), XMLoadFloat3(&upInitial))));
            dragCoeff = { 0.9f, 1.0f,  0.9f };
            //freeFall = true;
        }
    }

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

void Composer::AddBody(
    const ShapeType& type,
    const BodyProperties& props,
    const DirectX::XMFLOAT3& scales,
    const DirectX::XMFLOAT4& color,
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

    physicsEntitiesTrsfm[entitySize].color[0] = color.x;
    physicsEntitiesTrsfm[entitySize].color[1] = color.y;
    physicsEntitiesTrsfm[entitySize].color[2] = color.z;
    physicsEntitiesTrsfm[entitySize].color[3] = color.w;

}
