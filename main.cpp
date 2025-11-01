#include "window.hpp"
#include "Renderer/Renderer.hpp"

using namespace std;
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window wnd(1600, 900, L"yolo", L"test");
    Renderer renderer(hInstance, wnd.GetWindowHWND());

    while (wnd.ProcessMessages() == 0)
    {
        renderer.BeginRendering();
        renderer.Render();
        renderer.Present();
    }

}

