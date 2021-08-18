#ifndef NARRATIVE
#define NARRATIVE
#include <stdlib.h>
#include <stdint.h>

  struct applicationState
  {
    uint8_t syncStatus;
    uint8_t motorState;
    uint8_t currentPosition;
    uint8_t currentIndex;
    uint8_t currentDirection;
    long currentTime;
    long startTime;
    long motorMovementStartTime;
    uint32_t movementDelay;
    uint8_t applicationActive;
    uint8_t writeState;
    uint8_t readState;
    char filename[30];
    int * fd;
    FILE * fp;
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
    char log[256];
    int delayCountUsec;
    int motorProcessIdentifier;
  };

  // void *updateStateLog(void *state);


  #define NUMROWS 35
  #define NUMCOLS 120

#endif