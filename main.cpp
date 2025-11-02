#include "window.hpp"
#include "Renderer/Renderer.hpp"

using namespace std;
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window wnd(1600, 900, L"yolo", L"test");
    Renderer renderer(hInstance, wnd.GetWindowHWND());

    vector<GeometryEntry> boxGeo({ GeometryType::Box});
    uint64_t testCollection = renderer.CreateMeshCollection(boxGeo);

    while (wnd.ProcessMessages() == 0)
    {
        renderer.BeginRendering();
        renderer.Render(testCollection, 1, {});
        renderer.Present();
    }

}

