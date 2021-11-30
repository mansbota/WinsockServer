#pragma once

#include <chrono>

using namespace std::chrono;

class Timer
{

	 time_point<high_resolution_clock> timePoint;

public:

	Timer() { reset(); }

	void reset() {

		timePoint = high_resolution_clock::now();
	}

	double elapsed() const {

		duration<double> elapsedTime = high_resolution_clock::now() - timePoint;
		
		return elapsedTime.count() / 60.0 / 60.0;
	}
};

