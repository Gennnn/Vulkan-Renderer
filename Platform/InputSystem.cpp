#include "InputSystem.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

bool InputSystem::Initialize(GW::SYSTEM::GWindow _window) {
	window = _window;

	if (!+keyboardMouse.Create(window)) return false;
	if (!+controller.Create()) return false;

	return true;
}

bool InputSystem::IsWindowFocused() const {
	bool focused = false;
	window.IsFocus(focused);
	return focused;
}

float InputSystem::GetKeyState(int key) {
	float value = 0.0f;
	keyboardMouse.GetState(key, value);
	return value;
}

float InputSystem::GetMouseButtonState(int button) {
	float value = 0.0f;
	keyboardMouse.GetState(button, value);
	return value;
}

bool InputSystem::GetMouseDelta(float& outX, float& outY) {
	outX = 0.0f;
	outY = 0.0f;

	return keyboardMouse.GetMouseDelta(outX, outY) == GW::GReturn::SUCCESS;
}

float InputSystem::GetControllerState(int controllerIndex, int inputCode) {
	float value = 0.0f;
	controller.GetState(controllerIndex, inputCode, value);
	return value;
}

void InputSystem::CaptureCursor() {
#if defined(_WIN32)
	if (!IsWindowFocused()) return;

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE handle = {};
	window.GetWindowHandle(handle);

	HWND hwnd = static_cast<HWND>(handle.window);

	RECT client;
	GetClientRect(hwnd, &client);

	POINT ul{ client.left, client.top };
	POINT lr{ client.right, client.bottom };

	ClientToScreen(hwnd, &ul);
	ClientToScreen(hwnd, &lr);

	RECT clip{ ul.x, ul.y, lr.x, lr.y };
	ClipCursor(&clip);

	cursorCaptured = true;

	while (ShowCursor(FALSE) >= 0);
#endif
}

void InputSystem::ReleaseCursor() {
#if defined(_WIN32)
	ClipCursor(nullptr);
	cursorCaptured = false;
	while (ShowCursor(TRUE) < 0);
#endif
}

void InputSystem::CenterCursor() {
#if defined(_WIN32) 
	unsigned int width = 0;
	unsigned int height = 0;

	window.GetClientWidth(width);
	window.GetClientHeight(height);

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE handle = {};
	window.GetWindowHandle(handle);

	HWND hwnd = static_cast<HWND>(handle.window);

	POINT center;
	center.x = static_cast<LONG>(width * 0.5f);
	center.y = static_cast<LONG>(height * 0.5f);

	ClientToScreen(hwnd, &center);
	SetCursorPos(center.x, center.y);
#endif
}

void InputSystem::GetClientSize(unsigned int& width, unsigned int& height) const {
	window.GetClientWidth(width);
	window.GetClientHeight(height);
}