#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

uint64_t get_monotonic_msec();
uint32_t next_timer_ms();
void process_timers();

#endif // !TIMER_H
