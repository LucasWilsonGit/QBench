#pragma once

#ifndef TIMER_H //if this file needs to be copied, or compiled with a GCC version prior to GCC 3.4, an ifndef guard is necessary
#define TIMER_H 1

#include <chrono>
#include "core.h"

namespace Engine {
	//ended up doing the defs in-header since I intend to template the class for milliseconds or microseconds at some point, and the standard is to 
	class timer
	{
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
	public:
		timer() {
			m_startTime = std::chrono::high_resolution_clock::now();
		}
		~timer() {
			stop();
		}

		void restart() {
			m_startTime = std::chrono::high_resolution_clock::now();
		}

		void stop() {
			auto endTime = std::chrono::high_resolution_clock::now();

			auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTime).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTime).time_since_epoch().count();

			auto duration = end - start;

			CONSOLECOLOR(FOREGROUND_BLUE);
			std::cout << duration << std::endl;
			CONSOLECOLOR(FOREGROUND_WHITE);
		}
	};
}
#endif
