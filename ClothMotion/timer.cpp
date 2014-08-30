#include "Timer.h"

Timer::Timer (): last(0), total(0) {
	tick();
}

void Timer::tick () {
	time(&then);
}

void Timer::tock () {
	time_t now;
	time(&now);
	last = static_cast<double>(now - then);
	total += last;
	then = now;
}