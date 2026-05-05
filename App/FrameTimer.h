#pragma once

#include <chrono>

class FrameTimer {
public: 
	void Reset() {
		last = std::chrono::steady_clock::now();
	}

	float Tick() {
		const auto now = std::chrono::steady_clock::now();
		const float dt = std::chrono::duration<float>(now - last).count();
		last = now;
		return dt;
	}

private:
	std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
};