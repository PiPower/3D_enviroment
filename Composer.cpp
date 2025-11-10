#include "Composer.hpp"
#include "Renderer/CommonShapes.hpp"
#include <random>
using namespace DirectX;
using namespace std;

static constexpr XMFLOAT3 eyeInitial = { 0.0f, 1.0f, 0.0f };
static constexpr XMFLOAT3 lookDirInitial = { 0.0f, 0.0f, 1.0f };
static constexpr XMFLOAT3 upInitial = { 0.0f, 1.0f, 0.0f };
Composer::Composer(Renderer* renderer)
    :
    cameraAngleX(0), cameraAngleY(0), camOrientation(eyeInitial, upInitial, lookDirInitial), renderer(renderer)
{
    vector<GeometryEntry> boxGeo({ GeometryType::Box });
    renderer->CreateMeshCollection(boxGeo, &boxCollection);
    renderer->CreateUboPool(sizeof(Camera), sizeof(ObjectTransform), 300'000, &uboPool);
    renderer->AllocateUboResource(uboPool, UBO_CAMERA_RESOURCE_TYPE, &cameraUbo);
    renderer->BindUboPoolToPipeline((uint64_t)PipelineTypes::Graphics, uboPool, cameraUbo);


    XMStoreFloat4x4(&cameraBuffer.proj, XMMatrixPerspectiveFovLH(3.14f / 4.0f, 1600.0f / 900.0f, 0.1f, 90.0f));
    cameraBuffer.proj._22 *= -1.0f;// vulkan has -1.0f on top and 1.0f on bottom

    GenerateObjects();
}

void Composer::RenderScene()
{
    UpdateCamera();
    UpdateObjects();

    renderer->BeginRendering();
    renderer->Render(boxCollection, (uint64_t)PipelineTypes::Graphics, renderEntities);
    renderer->Present();
}

void Composer::UpdateCamera()
{
    XMVECTOR eyePos = XMLoadFloat3(&camOrientation.eye);
    XMVECTOR lookAt = XMLoadFloat3(&camOrientation.lookDir);
    XMVECTOR up = XMLoadFloat3(&camOrientation.up);
    XMStoreFloat4x4(&cameraBuffer.view, XMMatrixLookToLH(eyePos, lookAt, up));
    renderer->UpdateUboMemory(uboPool, cameraUbo, (char*)&cameraBuffer);
}

void Composer::GenerateObjects()
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0, 1.0);

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            ObjectTransform objTransform;
            XMStoreFloat4x4(&objTransform.transform, XMMatrixTranslation(-10 + i * 2, 0, -10 + j * 2));
            objTransform.color[0] = dis(gen);
            objTransform.color[1] = dis(gen);
            objTransform.color[2] = dis(gen);
            objTransform.color[3] = 1.0f;
            entities.push_back({ objTransform });

            uint64_t rectUbo;
            renderer->AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &rectUbo);
            renderEntities.push_back({ 0, 0, rectUbo, 0 });
        }
    }

    ObjectTransform objTransform;
    XMStoreFloat4x4(&objTransform.transform, XMMatrixScaling(20, 1, 20) * XMMatrixTranslation(0, -3, 0));
    objTransform.color[0] = dis(gen);
    objTransform.color[1] = dis(gen);
    objTransform.color[2] = dis(gen);
    objTransform.color[3] = 1.0f;
    entities.push_back({ objTransform });

    uint64_t rectUbo;
    renderer->AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &rectUbo);
    renderEntities.push_back({ 0, 0, rectUbo, 0 });
}

void Composer::UpdateObjects()
{
    for (size_t i = 0; i < entities.size(); i++)
    {
        renderer->UpdateUboMemory(uboPool, renderEntities[i].transformUboId, (char*) & entities[i].transform);
    }
}



void Composer::ProcessUserInput(Window* window, float dt)
{

    XMVECTOR eyeVec = XMLoadFloat3(&camOrientation.eye);
    XMVECTOR upVec = XMLoadFloat3(&camOrientation.up);
    XMVECTOR lookDirVec = XMLoadFloat3(&camOrientation.lookDir);

    if (window->IsKeyPressed('W')) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(lookDirVec, dt * 10.0f)); }
    if (window->IsKeyPressed('S')) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(lookDirVec, dt * 10.0f)); }
    if (window->IsKeyPressed(VK_SPACE)) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(upVec, dt * 10.0f)); }
    if (window->IsKeyPressed(VK_CONTROL)) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(upVec, dt * 10.0f)); }
    if (window->IsKeyPressed('D')) { eyeVec = XMVectorAdd(eyeVec, XMVectorScale(XMVector3Cross(upVec, lookDirVec), dt * 10.0f)); }
    if (window->IsKeyPressed('A')) { eyeVec = XMVectorSubtract(eyeVec, XMVectorScale(XMVector3Cross(upVec, lookDirVec), dt * 10.0f)); }
    if (window->IsLeftPressed())
    {
        static float angleX = 0.0f, angleY = 0.0f;
        if (window->GetMouseDeltaX() < 0)
        {
            angleY += dt * 10.0f;
        }
        else if (window->GetMouseDeltaX() > 0)
        {
            angleY += -dt * 10.0f;
        }

        if (window->GetMouseDeltaY() < 0)
        {
            angleX += -dt * 10.0f;
        }
        else if (window->GetMouseDeltaY() > 0)
        {
            angleX += dt * 10.0f;
        }
        // IMPORTANT: Order MATTERS !!!!
        XMMATRIX rotMatrix = XMMatrixRotationX(angleX) * XMMatrixRotationY(angleY);

        lookDirVec = XMVector3Transform(XMLoadFloat3(&lookDirInitial), rotMatrix);
        upVec = XMVector3Transform(XMLoadFloat3(&upInitial), rotMatrix);
    }

    XMStoreFloat3(&camOrientation.eye, eyeVec);
    XMStoreFloat3(&camOrientation.up, upVec);
    XMStoreFloat3(&camOrientation.lookDir, lookDirVec);
}
