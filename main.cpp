#include "window.hpp"
#include "Renderer/Renderer.hpp"

using namespace std;
using namespace DirectX;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window wnd(1600, 900, L"yolo", L"test");
    Renderer renderer(hInstance, wnd.GetWindowHWND());
    Camera camera = {};

    XMVECTOR eyePos, lookAt, up;
    XMFLOAT3 eyePosFloat{0.0f, 0.0f, -2}, lookAtFloat{0.0f, 0.0f, 1.0f}, upFloat{0.0f, 1.0f, 0.0f};
    eyePos = XMLoadFloat3(&eyePosFloat);
    lookAt = XMLoadFloat3(&lookAtFloat);
    up = XMLoadFloat3(&upFloat);

    XMStoreFloat4x4(&camera.view, XMMatrixLookToLH(eyePos, lookAt, up));
    XMStoreFloat4x4(&camera.proj, XMMatrixPerspectiveFovLH(3.14f / 4.0f, 1600.0f/900.0f, 0.1f, 90.0f));
    camera.proj._22 *= -1.0f;// vulkan has -1.0f on top and 1.0f on bottom

    vector<GeometryEntry> boxGeo({ GeometryType::Box});
    uint64_t testCollection, uboPool, cameraUbo, rectUbo;
    renderer.CreateMeshCollection(boxGeo, &testCollection);
    renderer.CreateUboPool(sizeof(Camera), sizeof(ObjectTransform), 0, &uboPool);
    renderer.AllocateUboResource(uboPool, UBO_CAMERA_RESOURCE_TYPE, &cameraUbo);
    renderer.AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &rectUbo);
    renderer.UpdateUboMemory(uboPool, cameraUbo, (char*) &camera);
    renderer.BindUboPoolToPipeline((uint64_t)PipelineTypes::Graphics, uboPool);
    while (wnd.ProcessMessages() == 0)
    {
        renderer.BeginRendering();
        renderer.Render(uboPool, cameraUbo,testCollection, (uint64_t)PipelineTypes::Graphics, {});
        renderer.Present();
    }

}



