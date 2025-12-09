#include "window.hpp"
#include "Renderer/Renderer.hpp"
#include "Composer.hpp"
#include "Physics/PhysicsEnigne.h"
#include <string>
#define LIGHTS 1
using namespace std;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window wnd(1600, 900, L"yolo", L"test");
    Renderer renderer(hInstance, wnd.GetWindowHWND(), LIGHTS);
    PhysicsEnigne physicsEngine{};
    std::vector<Light> lights = { Light{{0.67f, 0.34f, 0.99f, 1.0f}, {0.0f, 0.0f, 50.0f, 0.0f} } };
    Composer comp(&renderer, &physicsEngine, lights);


    float dt = 0;
    while (wnd.ProcessMessages() == 0)
    {
        auto t1 = chrono::high_resolution_clock::now();
        comp.ProcessUserInput(&wnd, dt);
        comp.UpdateObjects(dt);
        comp.RenderScene();

        auto t2 = chrono::high_resolution_clock::now();
        chrono::duration duration = t2 - t1;
        dt = (float)duration.count() / 1'000'000'000.0f;
    }

}


