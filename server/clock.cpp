#include "clock.h"
#include "config.h"
#include "serial.h"

unsigned long last_loop_debug;
unsigned long last_loop_idle;
unsigned long last_cmd_rx;
unsigned long t_started;
unsigned long stopwatch_started_0;
unsigned long stopwatch_started_1;
unsigned long stopwatch_started_2;

// TODO: make appropriate functions inline and put in header

int init_clock() {
    t_started = millis();
    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("CLK: -> t_started: %d", t_started);
    #endif
    last_loop_debug = 0;
    last_loop_idle = 0;
    last_cmd_rx = 0;
    return 0;
}

unsigned long delta_started() {
    return millis() - t_started;
}

void stopwatch_start_0() {
    stopwatch_started_0 = micros();
}

long stopwatch_stop_0() {
    return micros() - stopwatch_started_0;
}

void stopwatch_start_1() {
    stopwatch_started_1 = micros();
}

long stopwatch_stop_1() {
    return micros() - stopwatch_started_1;
}

void stopwatch_start_2() {
    stopwatch_started_1 = micros();
}

long stopwatch_stop_2() {
    return micros() - stopwatch_started_1;
}
