#include "Composer.hpp"
#include "Renderer/CommonShapes.hpp"
#include <random>
using namespace DirectX;
using namespace std;

static constexpr XMFLOAT3 eyeInitial = { -10.0f, 9.0f, -25.0f };
static constexpr XMFLOAT3 lookDirInitial = { 0.0f, 0.0f, 1.0f };
static constexpr XMFLOAT3 upInitial = { 0.0f, 1.0f, 0.0f };
static constexpr uint64_t graphicsPipelineType = (uint64_t)PipelineTypes::Graphics;
static constexpr size_t characterId = 0;

Composer::Composer(
    Renderer* renderer,
    PhysicsEnigne* physicsEngine,
    const std::vector<Light>& lights)
    :
    cameraAngleX(0), cameraAngleY(0), camOrientation(eyeInitial, upInitial, lookDirInitial), 
    renderer(renderer) , physicsEngine(physicsEngine), calculatePhysics(true), frameMode(true),
    lights(lights), characterVelocity({0, 0, 0})
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
        bodyProps.position = { -2, 1.07f, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 1.0f/40.f;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 0.0f;
        XMFLOAT3 scales = { 1.0f, 1.0f, 1.0f};
        XMFLOAT4 color = { 1.0f,  1.0f,  1.0f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, false);
    }
    // collider
    {
        BodyProperties bodyProps;
        bodyProps.position = { 10, 7, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 1.0f / 20.f;
        bodyProps.rotation = { 0, 0, 0, 1 };
        bodyProps.elasticity = 0.5f;
        XMFLOAT3 scales = { 1.0f, 1.0f, 1.0f };
        XMFLOAT4 color = { 1.0f,  1.0f,  1.0f, 1.0f };
        //AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
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
        XMFLOAT3 scales = { 35, 1, 35 };
        XMFLOAT4 color = { 0.2f, 0.5f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
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
        XMFLOAT3 scales = { 1, 20, 35 };
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
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
        XMFLOAT3 scales = { 1, 20, 35 };
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
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
        XMFLOAT3 scales = { 35, 20, 1 };
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
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
        XMFLOAT3 scales = { 35, 20, 1 };
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
    }


    // ramp
    {
        BodyProperties bodyProps;
        bodyProps.position = { 0, 0, 0 };
        bodyProps.linVelocity = { 0, 0, 0 };
        bodyProps.angVelocity = { 0, 0, 0 };
        bodyProps.massInv = 0;
        bodyProps.rotation = { 0, 0, 1.0f * sinf(-3.14/8), cosf(-3.14 / 8)};
        bodyProps.elasticity = 0.0f;
        XMFLOAT3 scales = { 5, 1, 1 };
        XMFLOAT4 color = { 0.5f, 0.1f, 0.1f, 1.0f };
        AddBody(ShapeType::OrientedBox, bodyProps, scales, color, true);
    }

}

void Composer::UpdateObjects(
    float dt)
{
    if (calculatePhysics)
    {
        physicsEngine->SetLinearVelocity(physicsEntities[characterId], X_COMPONENT | Y_COMPONENT, characterVelocity);
        characterVelocity = { 0, 0, 0 };

        physicsEngine->UpdateBodies(dt);
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
    if (window->IsKeyPressed(VK_UP)) { characterVelocity.z += 10.0f; }
    if (window->IsKeyPressed(VK_DOWN)) { characterVelocity.z += -10.0f; }
    if (window->IsKeyPressed(VK_LEFT)) { characterVelocity.x += -10.0f; }
    if (window->IsKeyPressed(VK_RIGHT)) { characterVelocity.x += 5.0f; characterVelocity.y += 5.0f;}
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
    bool allowAngularImpulse)
{
    uint64_t uboId;
    renderer->AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &uboId);
    renderEntities.push_back({ 0, 0, uboId, 0 });

    size_t entitySize = physicsEntities.size();
    physicsEntities.push_back(0);
    physicsEntitiesTrsfm.push_back({});

    bool isDynamic = props.massInv != 0.0f;
    physicsEngine->AddBody(props, type, scales, isDynamic, &physicsEntities[entitySize], allowAngularImpulse);
    physicsEngine->GetTransformMatrixForBody(physicsEntities[entitySize], &physicsEntitiesTrsfm[entitySize].transform);

    physicsEntitiesTrsfm[entitySize].color[0] = color.x;
    physicsEntitiesTrsfm[entitySize].color[1] = color.y;
    physicsEntitiesTrsfm[entitySize].color[2] = color.z;
    physicsEntitiesTrsfm[entitySize].color[3] = color.w;

}
