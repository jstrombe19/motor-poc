#ifndef NARRATIVE
#define NARRATIVE
#include <stdlib.h>
#include <stdint.h>

  struct applicationState
  {
    uint8_t syncStatus;
    uint8_t motorState;
    uint32_t currentPosition;
    uint32_t currentIndex;
    uint32_t homePosition;
    uint32_t homeIndex;
    uint32_t lastPosition;
    uint32_t lastIndex;
    uint8_t currentDirection;
    char currentTime[80];
    long startTime;
    time_t motorMovementStartTime;
    time_t motorMovementElapsedTime;
    uint32_t movementDelay;
    uint8_t applicationActive;
    uint8_t writeState;
    uint8_t readState;
    char filename[30];
    int * fd;
    FILE * fp;
    FILE * pfp;
    int desiredOutRate;
    int directionOfRotation;
    float changeInAngularPosition;
    uint8_t state;
    uint8_t motorMode;
    uint8_t loadStep;
    uint16_t phaseStepSize;
    uint32_t numberOfSteps;
    uint32_t stepDelay;
    uint8_t commandSequence[15];
    char *log;
    int delayCountUsec;
    int motorProcessIdentifier;
    int motorMovementPending;
    int performanceCycleCount;
    int cycleCount;
    int16_t picoDevice;
    int newFileQueue;
    uint32_t hardStopMin;
    uint32_t hardStopMax;

  };

  // void *restartReadLog(void *state);
  // void *updateStateLog(void *state);


  #define NUMROWS 35
  #define NUMCOLS 120

#endif