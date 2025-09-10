#pragma once

namespace Utils::Timer {

	struct Timer {

		Timer() { lastDt = std::chrono::high_resolution_clock::now(); }
		int count = 0;
		std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

		void tick();
		float frameDeltaTime();
		float getFPS();

	private:

		std::chrono::high_resolution_clock::time_point lastDt;
		float delta = 0.0f;

		std::chrono::high_resolution_clock::time_point lastCheckedFPS = std::chrono::high_resolution_clock::now();
		int framesPerCycle = 0;
	};

	enum class TimerType {

		MainWindow,
		Other
	};

	// static map of counter structs to store each one...given a name passed
	inline std::array<Timer, 2> timers;  

	inline Timer& get(TimerType type = TimerType::MainWindow) {

		std::cout << "Address of ref MINA!!: " << &timers << "\n";
		std::cout << "Address of ref MINA2!!: " << &(timers[static_cast<std::size_t>(type)]) << "\n";
		return timers[static_cast<std::size_t>(type)];
	}
}