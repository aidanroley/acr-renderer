#pragma once

namespace Debug::Timer::CPS {

	struct Counter {

		int count = 0;
		std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

		void tick() {

			count++;
			auto now = std::chrono::high_resolution_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count();

			if (diff >= 1) {

				std::cout << "Calls per second: " << count << std::endl;
				count = 0;
				lastTime = now;
			}
		}
	};
	// static map of counter structs to store each one...given a name passed
	inline Counter& get(const char* name = "default") {

		static std::unordered_map<std::string, Counter> map;
		return map[name];
	}
}