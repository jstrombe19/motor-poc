#ifndef CONFIG_H
#define CONFIG_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t BAUD_RATE;
#define BAUD_RATE 115200
#define DEVICE "/dev/ttyUSB0"
#define PORT "ttyUSB0"
#define SYNC_SIGNAL 0xA5CAFEA5
#define SYNC_BYTE 0xA5
#define MOTOR_TYPE 1 // configurable to support the development motor, which is type `0`
#define MOTOR_STEPS_PER_REVOLUTION 9216
#define MOTOR_BACKLASH_COMPENSATION 1.166

#define LIFECYCLE_RATE_OF_ROTATION 20
#define LIFECYCLE_CYCLE_ITERATION 200
#define RATE_OF_ROTATION 10
#define PERFORMANCE_RATE_OF_ROTATION 5

#define PERFORMANCE_HOLD 1000000  // usec
#define DELAY_BUFFER_FACTOR 1.00

#define LIFECYCLE_DISPLACEMENT 60
#define LWIR_DROT 55.58
#define MWIR_DROT -56.55
#define SWIR_DROT -60.19
#define VIS_DROT 60.18
#define CW_BUMP 65.791
#define CCW_BUMP -65.791
#define BUMP_AMPLIFIER 1.12
#define CCW_BUMP_TO_CW_BUMP 131.591796875

#define BACKLASH 10

#endif