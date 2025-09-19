#include "pch.h"
#include "Core/Utils/timer.h"

namespace Utils::Timer {

	float Timer::frameDeltaTime() {

		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> d = now - lastDt;
		lastDt = now;
		delta = d.count(); // count defaults to tick being second
		return delta;
	}

	float Timer::getFPS() {

		auto now = std::chrono::high_resolution_clock::now();
		float diff = std::chrono::duration<float>(now - lastCheckedFPS).count();
		framesPerCycle++;

		if (diff >= 0.7f) {

			lastCheckedFPS = now;
			lastFPS = framesPerCycle / diff; // frames per second
			framesPerCycle = 0;
			
		}
		return lastFPS;
	}

	void Timer::tick() {

		count++;
		auto now = std::chrono::high_resolution_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count();

		if (diff >= 1) {

			std::cout << "Calls per second: " << count << std::endl;
			count = 0;
			lastTime = now;
		}
	}
}