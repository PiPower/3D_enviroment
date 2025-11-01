#include "window.hpp"
#include <Windowsx.h>
#include <string>
#include <assert.h>
#include <strsafe.h>
using namespace std;
using namespace std::chrono;

Window::Window(int width, int height, std::wstring CLassName, std::wstring WndName)
	:Hinstace(GetModuleHandle(nullptr)), width(width), height(height), CLASS_NAME(CLassName), WindowName(WndName)
{
	WNDCLASSEX  wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLassName.c_str();
	wc.hIconSm = nullptr;

	RegisterClassEx(&wc);

	DWORD style = WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX;

	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	AdjustWindowRect(&wr, style, FALSE);

	CreateWindowEx(0, CLassName.c_str(), WndName.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, GetModuleHandle(nullptr), this);


	ShowWindow(hwnd, SW_SHOWDEFAULT);
	KeyStates.reset();

	old = high_resolution_clock::now();
	fpsTick = 0;
}

int Window::ProcessMessages() noexcept
{
	MSG msg;
	const duration<float> frameTime = high_resolution_clock::now() - old;
	float frameInterval = frameTime.count();
	if (frameInterval > 1.0)
	{
		old = high_resolution_clock::now();

		fpsInfo.clear();

		fpsInfo = WindowName + L" FPS: " + to_wstring(fpsTick);
		SetWindowText(hwnd, fpsInfo.c_str());

		fpsTick = 0;
	}
	fpsTick++;
	deltaX = 0;
	deltaY = 0;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return msg.wParam;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

HWND Window::GetWindowHWND()
{
	return this->hwnd;
}

void Window::SetFPS()
{
	while (!IsMouseEventEmpty())
	{
		auto e = ReadMouseEvent();

		if (e.Type == MouseEvent::Event::Move)
		{
			fpsInfo = L"(" + to_wstring(e.x) + L',' + to_wstring(e.y) + L')' + L"(" +
				to_wstring(GetMousePosXNormalized()) + L', ' + to_wstring(GetMousePosYNormalized());
			SetWindowText(hwnd, fpsInfo.c_str());
		}

	}

}

void Window::DrawMessageBox(std::wstring mes)
{
	MessageBox(hwnd, mes.c_str(), NULL, MB_OK);
}


bool Window::IsKeyPressed(unsigned char key)
{
	return KeyStates[key];
}

wchar_t Window::GetChar()
{
	if (CharQueue.size() > 0)
	{
		const wchar_t c = CharQueue.front();
		CharQueue.pop();
		return c;
	}
	else return 0;
}

bool Window::IsCharEmpty()
{
	return CharQueue.empty();
}

bool Window::IsKeyboardEventEmpty()
{
	return KeyQueue.empty();
}

Window::KeyEvent Window::ReadKeyEvent()
{
	if (KeyQueue.size() > 0)
	{
		KeyEvent k = KeyQueue.front();
		KeyQueue.pop();
		return k;
	}
	else return KeyEvent(0, KeyEvent::Event::Invalid);
}

void Window::ClearKeyEvent()
{
	while (!IsKeyboardEventEmpty())
	{
		KeyQueue.pop();
	}
}

void Window::ClearCharQueue()
{
	while (!IsCharEmpty())
	{
		CharQueue.pop();
	}
}

bool Window::IsRightPressed() noexcept
{
	return RightPress;
}

bool Window::IsLeftPressed() noexcept
{
	return LeftPress;
}

bool Window::IsMiddlePressed() noexcept
{
	return MiddlePress;
}

int Window::GetMousePosX() noexcept
{
	return PosX;
}

int Window::GetMousePosY() noexcept
{
	return PosY;
}

int Window::GetMouseDeltaX() noexcept
{
	return deltaX;
}

int Window::GetMouseDeltaY() noexcept
{
	return deltaY;
}

float Window::GetMousePosXNormalized() noexcept
{
	return  ((float)PosX / (width / 2.0f) - 1.0f);
}

float Window::GetMousePosYNormalized() noexcept
{
	return  (-(float)PosY / (height / 2.0f) + 1.0f);
}

bool Window::IsMouseEventEmpty()
{
	return MouseQueue.empty();
}

Window::MouseEvent Window::ReadMouseEvent()
{
	if (MouseQueue.size() > 0)
	{
		MouseEvent m = MouseQueue.front();
		MouseQueue.pop();
		return m;
	}
	else return MouseEvent(0, 0, MouseEvent::Event::Invalid);
}

void Window::ClearMouseQueue()
{
	while (!IsMouseEventEmpty())
	{
		MouseQueue.pop();
	}
}


void Window::SimpleTypeBox()
{
	HWND  g_hPrzycisk = CreateWindowEx(0, L"BUTTON", L"Checkbox", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 100, 100, 150, 30, hwnd, NULL, Hinstace, NULL);
	ShowWindow(g_hPrzycisk, SW_SHOWDEFAULT);

}

void Window::RegisterResizezable(void* object, void(*func)(HWND, void*))
{
	resizableObjects.push_back({ object, func });
}


LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	Window* pWnd = nullptr;
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		pWnd->hwnd = hWnd;
	}
	else
	{
		pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}


	if (pWnd)
		return pWnd->HandleMsg(msg, wParam, lParam);
	else
		return DefWindowProc(hWnd, msg, wParam, lParam);
}


LRESULT Window::HandleMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(2);
		return 0;
		// Keboard------------------------------------------------------------------------
	case WM_KILLFOCUS:
		KeyStates.reset();
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		KeyStates[(unsigned char)wParam] = true;
		KeyQueue.emplace((unsigned char)wParam, KeyEvent::Event::Press);
		if (KeyQueue.size() > QueueLimit) KeyQueue.pop();
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		KeyStates[(unsigned char)wParam] = false;
		KeyQueue.emplace((unsigned char)wParam, KeyEvent::Event::Release);
		if (KeyQueue.size() > QueueLimit) KeyQueue.pop();
		break;
	case WM_CHAR:
		CharQueue.push((wchar_t)wParam);
		if (CharQueue.size() > QueueLimit) CharQueue.pop();
		break;
		// Mouse------------------------------------------------------------------------
	case WM_LBUTTONDOWN:
		LeftPress = true;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::LeftPress);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_LBUTTONUP:
		LeftPress = false;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::LeftRelease);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_MBUTTONDOWN:
		MiddlePress = true;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::WheelPrees);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_MBUTTONUP:
		MiddlePress = true;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::WheelRelease);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_RBUTTONDOWN:
		RightPress = true;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::RightPress);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_RBUTTONUP:
		RightPress = true;
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::RightRelease);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_MOUSEMOVE:
		deltaX = PosX - GET_X_LPARAM(lParam);
		deltaY = -(PosY - GET_Y_LPARAM(lParam));
		PosX = GET_X_LPARAM(lParam);
		PosY = GET_Y_LPARAM(lParam);
		MouseQueue.emplace(PosX, PosY, MouseEvent::Event::Move);
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	case WM_SIZE:
		for (int i = 0; i < resizableObjects.size(); i++)
		{
			resizableObjects[i].func(this->hwnd, resizableObjects[i].ptr);
		}
		break;
	case WM_MOUSEWHEEL:
		ScrollDelta = 0;
		ScrollAcumulate = GET_WHEEL_DELTA_WPARAM(wParam);
		if ((ScrollAcumulate / 120 >= 1) || (ScrollAcumulate / 120 <= -1))
		{
			ScrollDelta = ScrollAcumulate / 120;
			MouseEvent::Event t;
			if (ScrollDelta > 0) t = MouseEvent::Event::WheelUp;
			else  t = MouseEvent::Event::WheelDown;
			MouseQueue.emplace(ScrollDelta, 0, t);
			ScrollAcumulate = 0;
		}
		if (MouseQueue.size() > QueueLimit)MouseQueue.pop();
		break;
	default:
		return DefWindowProc(this->hwnd, msg, wParam, lParam);
	}

	return 0;
}
