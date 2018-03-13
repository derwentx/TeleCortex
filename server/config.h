#ifndef __CONFIG_H__
#define __CONFIG_H__

// Serial Baud rate
#define SERIAL_BAUD 57600

// debug flags
#define DEBUG 0
#define DEBUG_PANEL 0
#define DEBUG_LOOP 1
#define DEBUG_GCODE 0
#define DEBUG_TIMING 0
#define DEBUG_QUEUE 0

/* Command processing flags */
#define REQUIRE_CHECKSUM 1
#define REQUIRE_CONSECUTIVE_LINENUM 1

/* Display rainbows until receive GCode */
#define RAINBOWS_UNTIL_GCODE 1

// Maximum command length
#define MAX_CMD_SIZE 260

// Number of commands in the queue
#define MAX_QUEUE_LEN 20

// TIMING
#define LOOP_WAIT_PERIOD 1
#define LOOP_IDLE_PERIOD 100
#define LOOP_DEBUG_PERIOD 1000
#define FAIL_WAIT_PERIOD 10

#include "panel_config.h"
#include "board_properties.h"

#endif /* __CONFIG_H__ */
