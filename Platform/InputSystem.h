#pragma once

#include "../Core/GatewareConfig.h"

class InputSystem {
public:
	bool Initialize(GW::SYSTEM::GWindow window);

	bool IsWindowFocused() const;

	float GetKeyState(int key);
	float GetMouseButtonState(int button);
	bool GetMouseDelta(float& outX, float& outY);

	float GetControllerState(int controllerIndex, int inputCode);

	bool IsCursorCaptured() const { return cursorCaptured; }

	void CaptureCursor();
	void ReleaseCursor();
	void CenterCursor();

	void GetClientSize(unsigned int& width, unsigned int& height) const;

private:
	GW::SYSTEM::GWindow window;
	GW::INPUT::GInput keyboardMouse;
	GW::INPUT::GController controller;

	bool cursorCaptured = false;
};