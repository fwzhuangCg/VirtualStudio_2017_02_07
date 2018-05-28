// wunf 's timer class
#ifndef __TIMER_H
#define __TIMER_H

#include <time.h>

struct Timer {
	time_t then;
	double last, total;
	Timer ();
	void tick (), tock ();
};
#endif
