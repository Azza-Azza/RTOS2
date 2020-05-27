#include "timer.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

// TIMER
unsigned int period = 500000;

#ifdef __GNUC__
void __attribute__((weak)) TICK_Handler() {}
#endif

void timerHandler(int a) {
    TICK_Handler();
    ualarm(period, 0);
}
void fakeuc_enableTimerInterrupts() { 
    signal(SIGALRM, &timerHandler);
    ualarm(period, 0);
}