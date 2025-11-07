#include "window.hpp"
#include "Renderer/Renderer.hpp"

using namespace std;
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window wnd(1600, 900, L"yolo", L"test");
    Renderer renderer(hInstance, wnd.GetWindowHWND());

    vector<GeometryEntry> boxGeo({ GeometryType::Box});
    uint64_t testCollection, uboPool, cameraUbo, rectUbo;
    renderer.CreateMeshCollection(boxGeo, &testCollection);
    renderer.CreateUboPool(sizeof(Camera), sizeof(ObjectTransform), 1000000, &uboPool);
    renderer.AllocateUboResource(uboPool, UBO_CAMERA_RESOURCE_TYPE, &cameraUbo);
    renderer.AllocateUboResource(uboPool, UBO_OBJ_TRSF_RESOURCE_TYPE, &rectUbo);

    while (wnd.ProcessMessages() == 0)
    {
        renderer.BeginRendering();
        renderer.Render(testCollection, (uint64_t)PipelineTypes::Graphics, {});
        renderer.Present();
    }

}

