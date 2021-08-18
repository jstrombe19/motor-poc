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

#define RATE_OF_ROTATION 10
#define PERFORMANCE_RATE_OF_ROTATION 5
#define LWIR_DROT 58.58
#define MWIR_DROT -56.55
#define SWIR_DROT -60.19
#define VIS_DROT 60.18
#define CW_BUMP 66.01
#define CCW_BUMP -66.01

#endif