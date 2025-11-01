#pragma once

#ifndef UNICODE
#define UNICODE
#endif 

#include <Windows.h>
#include <bitset>
#include <queue>
#include <algorithm>
#include <objidl.h>
#include <gdiplusenums.h>
#include <gdiplus.h>
#include <chrono>


typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

class Window
{
public:
	struct KeyEvent
	{

		enum class Event
		{
			Press,
			Release,
			Invalid
		};
		KeyEvent(unsigned char code, Event type) noexcept
			:
			Code(code), Type(type)
		{}
		const unsigned char Code;
		const Event Type;
	};

	struct MouseEvent
	{

		enum class Event
		{
			LeftPress,
			LeftRelease,
			RightPress,
			RightRelease,
			WheelPrees,
			WheelRelease,
			WheelUp,
			WheelDown,
			Move,
			Invalid
		};
		MouseEvent(int x, int y, Event type) noexcept
			:
			x(x), y(y), Type(type)
		{}
		const int x;
		const int y;
		const Event Type;
	};
private:
	struct RegisteredObject
	{
		void* ptr;
		void(*func)(HWND, void*);
		RegisteredObject() = default;
	};
public:
	Window(int width, int height, std::wstring CLassName, std::wstring WndName);
	int ProcessMessages() noexcept;
	HWND GetWindowHWND();
	void SetFPS();
	void DrawMessageBox(std::wstring mes);
	// Keyboard ------------------------------------
	bool IsKeyPressed(unsigned char key);
	wchar_t GetChar();
	bool IsCharEmpty();
	bool IsKeyboardEventEmpty();
	Window::KeyEvent ReadKeyEvent();
	void ClearKeyEvent();
	void ClearCharQueue();
	// Mouse ---------------------------------------
	bool IsRightPressed() noexcept;
	bool IsLeftPressed() noexcept;
	bool IsMiddlePressed() noexcept;
	int GetMousePosX() noexcept;
	int GetMousePosY() noexcept;
	int GetMouseDeltaX() noexcept;
	int GetMouseDeltaY() noexcept;
	float GetMousePosXNormalized() noexcept;
	float GetMousePosYNormalized() noexcept;
	bool IsMouseEventEmpty();
	Window::MouseEvent ReadMouseEvent();
	void ClearMouseQueue();
	void SimpleTypeBox();
	void RegisterResizezable(void* object, void(*func)(HWND, void*));
private:
	static LRESULT CALLBACK  HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT HandleMsg(UINT msg, WPARAM wParam, LPARAM lParam);
private:
	std::wstring CLASS_NAME;
	HINSTANCE Hinstace;
	HWND hwnd;
	int width;
	int height;
	int QueueLimit = 13;
	// Keyboard ------------------------------------
	std::bitset<256> KeyStates;
	std::queue<wchar_t> CharQueue;
	std::queue<KeyEvent> KeyQueue;
	// Mouse ---------------------------------------
	bool LeftPress = false;
	bool RightPress = false;
	bool MiddlePress = false;
	int ScrollAcumulate = 0;
	int ScrollDelta;
	int PosX;
	int PosY;
	int deltaX;
	int deltaY;
	std::queue<MouseEvent> MouseQueue;
	// Graphics ------------------------------------
	std::wstring fpsInfo;
	std::wstring WindowName;
	TimePoint old;
	unsigned int fpsTick;
	std::vector<RegisteredObject> resizableObjects;
};