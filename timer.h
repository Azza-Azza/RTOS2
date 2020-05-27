#ifndef _timer_h
#define _timer_h
#ifdef __cplusplus
#pragma error "Using C++ is not allowed for this assignment."
extern "C" {
#endif // __cplusplus

// TIMER
    // Timer interrupt period in microseconds
    // DEFAULT: 500,000 µs (0.5s), max allowed is 999,999µS
    extern unsigned int period; 

    // Enable Timer interrupts
    void TICK_Handler();
    void fakeuc_enableTimerInterrupts();

// ---

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _timer_h