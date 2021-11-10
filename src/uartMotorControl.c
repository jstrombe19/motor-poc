#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "../inc/config.h"
#include "./serial.h"
#include "./readUART.h"
#include "./writeUART.h"
#include "./narrative.h"


pthread_mutex_t lock;
static uint32_t localCurrentPosition;



void *collectUserInput(void *state) {
  struct applicationState *stateptr = (void *)state;
  // char *prompt = "Please enter the desired angular velocity in degrees per second";
  strcpy(stateptr->log, "Please enter the desired angular velocity in degrees per second");
  // printf("\nPlease enter the desired angular velocity in degrees per second\n");
  scanf("%d", &stateptr->desiredOutRate);
  strcpy(stateptr->log, "Enter the desired displacement in degrees");
  // printf("\nHow far do you want to rotate (in degrees)?\n");
  scanf("%f", &stateptr->changeInAngularPosition);
  return 0;
}


int reset_command_values(uint8_t * command) {
  for(int i = 2; i < sizeof(command); i++) {
    command[i] = 0;
  }
  return 0;
}


void *calculateOffset(void *state) {
  struct applicationState *stateptr = (void *)state;

  if (stateptr->currentPosition > 0x16000) {
    stateptr->changeInAngularPosition = (0xffffffff - stateptr->currentPosition - stateptr->homePosition) / 204.8;
  } else {
    stateptr->changeInAngularPosition = (stateptr->currentPosition - stateptr->homePosition) / 204.8;
    stateptr->changeInAngularPosition = stateptr->changeInAngularPosition * (-1);
  }

  return NULL;
}


uint32_t correctPositionValue(uint32_t positionValue) {
  if (positionValue > 0x16000) {
    positionValue = 0xffffffff - positionValue;
  }
  return positionValue;
}


void splitHardStops(void *state) {
  struct applicationState *stateptr = (void *)state;
  uint32_t tempMin = correctPositionValue(stateptr->hardStopMin);
  uint32_t tempMax = correctPositionValue(stateptr->hardStopMax);

  stateptr->changeInAngularPosition = (abs(tempMax) + abs(tempMin)) / 2 / 204.8 * (-1);
  return;
}


void *resetEncoder(void *state) {
  struct applicationState *stateptr = (void *) state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 7;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  usleep(100000);
  stateptr->state = 15;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  stateptr->homePosition = stateptr->currentPosition;
  stateptr->homeIndex = stateptr->currentIndex;
  return NULL;
}


void *encoderOnline(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->motorState = 0;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 8;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *powerOnMotor(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->motorState = 1;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 15;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *loadStepDirection(void *state) {
  struct applicationState *stateptr = (void *)state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 15;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *loadMovementCommand(void *state) {
  struct applicationState *stateptr = (void *)state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 15;
  convertInputToUartValues((void*)stateptr);
  stateptr->loadStep = 1;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


int printCommandQueueValues(uint8_t * command) {
  for(int i = 0; i < sizeof(command); i++) {
    printf("command position %d: %x\n", i, command[i]);
  }
  return 0;
}


void *moveMotor(void *state) {
  pthread_mutex_lock(&lock);
  struct applicationState *stateptr = (void *)state;
  stateptr->motorState = 2;

  switch (stateptr->motorProcessIdentifier) {
    case 0:
      while (stateptr->motorProcessIdentifier == 0) {
        NULL;
      }
    case 1:
      stateptr->desiredOutRate = LIFECYCLE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = -LIFECYCLE_DISPLACEMENT;
      break;
    case 2:
      stateptr->desiredOutRate = LIFECYCLE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = LIFECYCLE_DISPLACEMENT * 2;
      break;
    case 3:
      stateptr->desiredOutRate = LIFECYCLE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = -LIFECYCLE_DISPLACEMENT;
      break;
    case 4:
      // collectUserInput((void *)stateptr);
      stateptr->desiredOutRate = RATE_OF_ROTATION;
      break;
    case 5:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = CCW_BUMP_TO_CW_BUMP * BUMP_AMPLIFIER * (-1);
      break;
    case 6:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = CCW_BUMP_TO_CW_BUMP * BUMP_AMPLIFIER;
      break;
    case 7:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      splitHardStops((void *)stateptr);
      break;
    case 8:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      calculateOffset((void *)stateptr);
      break;
    default:
      printf("Invalid movement process identifier: %d is undefined", stateptr->motorProcessIdentifier);
      break;
  }

  stateptr->delayCountUsec = abs((int)(((stateptr->changeInAngularPosition / stateptr->desiredOutRate) * DELAY_BUFFER_FACTOR) * 1000 * 1000));

  powerOnMotor((void *)stateptr);
  loadStepDirection((void *)stateptr);
  loadMovementCommand((void *)stateptr);
  usleep((stateptr->delayCountUsec) + PERFORMANCE_HOLD);

  stateptr->motorState = 1;
  
  pthread_mutex_unlock(&lock);
  return NULL;

}


void *functionalManeuver(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 1;
  moveMotor((void *)stateptr);
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 2;
  moveMotor((void *)stateptr);
  stateptr->motorMovementStartTime = time(NULL);
  stateptr->motorProcessIdentifier = 3;
  moveMotor((void *)stateptr);
  stateptr->motorProcessIdentifier = 0;
  stateptr->changeInAngularPosition = 0;
  stateptr->numberOfSteps = 0;
  return NULL;
}


void *performanceManeuver(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->homeIndex = stateptr->currentIndex;
  resetEncoder((void *)state);
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 5;
  localCurrentPosition = stateptr->currentPosition;
  moveMotor((void *)stateptr);
  stateptr->hardStopMin = stateptr->currentPosition;
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 6;
  moveMotor((void *)stateptr);
  stateptr->hardStopMax = stateptr->currentPosition;
  stateptr->motorMovementStartTime = time(NULL);
  stateptr->motorProcessIdentifier = 7;
  moveMotor((void *)stateptr);
  stateptr->motorProcessIdentifier = 0;
  stateptr->changeInAngularPosition = 0;
  stateptr->numberOfSteps = 0;
  return NULL;
}


void *stopMotorMovement(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
  if (stateptr->directionOfRotation == 3) {
    stateptr->changeInAngularPosition = -0.000001;
  } else {
    stateptr->changeInAngularPosition = 0.0;
  }
  loadMovementCommand((void *)stateptr);
  return NULL;
}


void *motorMoveMonitor(void *state) {
  struct applicationState *stateptr = (void *)state;
  while (stateptr->applicationActive) {
    switch (stateptr->motorMovementPending)
    {
    case 0:
      break;
    case 1:
      performanceManeuver((void *)stateptr);
      stateptr->motorMovementPending = 0;
      stateptr->performanceCycleCount += 1;
      break;
    case 2:
      functionalManeuver((void *)stateptr);
      stateptr->motorMovementPending = 0;
      stateptr->cycleCount += 1;
      break;
    case 3:
      stateptr->motorProcessIdentifier = 4;
      moveMotor((void *)stateptr);
      stateptr->motorMovementPending = 0;
    case 4:
      stateptr->motorProcessIdentifier = 8;
      moveMotor((void *)stateptr);
      stateptr->motorMovementPending = 0;
    case 5:
      while (stateptr->motorMovementPending == 5) {
        performanceManeuver((void *)stateptr);
        stateptr->performanceCycleCount += 1;
        int i = 0;
        while (i < LIFECYCLE_CYCLE_ITERATION && stateptr->motorMovementPending == 5) {
          functionalManeuver((void *)stateptr);
          stateptr->cycleCount += 1;
          stateptr->newFileQueue = 1;
          i++;
        }
      }
      break;
    case 6:
      stateptr->motorProcessIdentifier = 4;
      stateptr->changeInAngularPosition = LWIR_DROT;
      moveMotor((void *)stateptr);
      stateptr->motorMovementPending = 0;
      break;
    case 7:
      stateptr->motorProcessIdentifier = 4;
      stateptr->changeInAngularPosition = MWIR_DROT;
      moveMotor((void *)stateptr);
      stateptr->motorMovementPending = 0;
      break;
    case 8:
      stateptr->motorProcessIdentifier = 8;
      moveMotor((void *)stateptr);
      stateptr->motorMovementPending = 0;
      break;
    default:
      break;
    }
  }
  return NULL;
}


// todo: add clearing out of steps, angular displacement, d_rot => readUART.c values
void *motorMovementOverwatch(void *state) {
  struct applicationState *stateptr = (void *)state;
  
  while(stateptr->applicationActive) {

    if (stateptr->abort) {
      stopMotorMovement((void *)stateptr);
      stateptr->abort = 0;
    }

    if (stateptr->motorProcessIdentifier == 5) {
      while (stateptr->motorProcessIdentifier == 5 && stateptr->currentPosition == localCurrentPosition) {
        delay(100000);
      }
      while (stateptr->motorProcessIdentifier == 5 && stateptr->currentPosition > stateptr->lastPosition) {
        delay(100000);
      }
      while (stateptr->motorProcessIdentifier == 5 && stateptr->currentPosition < stateptr->lastPosition) {
        delay(100);
      }
      stopMotorMovement((void *)stateptr);
      localCurrentPosition = stateptr->currentPosition;
      stateptr->motorProcessIdentifier = 0;
    }

    if (stateptr->motorProcessIdentifier == 6) {
      while (stateptr->motorProcessIdentifier == 6 && stateptr->currentPosition == localCurrentPosition) {
        delay(100000);
      }
      while (stateptr->motorProcessIdentifier == 6 && stateptr->currentPosition > 0x12000) {
        delay(100000);
      }
      while (stateptr->motorProcessIdentifier == 6 && stateptr->currentPosition < stateptr->lastPosition) {
        delay(100000);
      }
      while (stateptr->motorProcessIdentifier == 6 && stateptr->currentPosition > stateptr->lastPosition) {
        delay(100000);
      }
      stopMotorMovement((void *)stateptr);
      stateptr->motorProcessIdentifier = 0;
    }
  }
  return NULL;
}