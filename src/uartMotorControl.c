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
  // uint32_t tempHome;
  // uint32_t tempCurrent;
  // if (stateptr->currentPosition > 0x16000) {
  //   tempCurrent = 0xffffffff - stateptr->currentPosition;
  // } else {
  //   tempCurrent = stateptr->currentPosition;
  // }
  // if (stateptr->homePosition > 0x16000) {
  //   tempHome = 0xffffffff - stateptr->homePosition;
  // } else {
  //   tempHome = stateptr->homePosition;
  // }


  if (stateptr->currentPosition > 0x16000) {
    stateptr->changeInAngularPosition = (0xffffffff - stateptr->currentPosition - stateptr->homePosition) / 204.8;
  } else {
    stateptr->changeInAngularPosition = (stateptr->currentPosition - stateptr->homePosition) / 204.8;
    stateptr->changeInAngularPosition = stateptr->changeInAngularPosition * (-1);
  }



  // if (stateptr->currentPosition > 0x16000 && stateptr->currentPosition > stateptr->homePosition && stateptr->homePosition > 0x16000) {
  //   stateptr->changeInAngularPosition = -(tempHome - tempCurrent) / 204.8;
  // } else if (stateptr->currentPosition > 0x16000 && stateptr->currentPosition > stateptr->homePosition && stateptr->homePosition < 0x16000) {
  //   stateptr->changeInAngularPosition = (tempHome + tempCurrent) / 204.8;
  // } else if (stateptr->currentPosition > 0x16000 && stateptr->currentPosition < stateptr->homePosition && stateptr->homePosition > 0x16000) {
  //   stateptr->changeInAngularPosition = (tempCurrent - tempHome) / 204.8;
  // } else if (stateptr->currentPosition < 0x16000 && stateptr->currentPosition > stateptr->homePosition && stateptr->homePosition < 0x16000) {
  //   stateptr->changeInAngularPosition = -(tempCurrent - tempHome) / 204.8;
  // } else if (stateptr->currentPosition < 0x16000 && stateptr->currentPosition < stateptr->homePosition && stateptr->homePosition > 0x16000) {
  //   stateptr->changeInAngularPosition = -(tempHome + tempCurrent) / 204.8;
  // } else if (stateptr->currentPosition < 0x16000 && stateptr->currentPosition < stateptr->homePosition && stateptr->homePosition < 0x16000) {
  //   stateptr->changeInAngularPosition = (tempHome - tempCurrent) / 204.8;
  // }
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

  stateptr->changeInAngularPosition = (abs(tempMax - tempMin)) / 2 / 204.8 * (-1);
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
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 8;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *powerOnMotor(void *state) {
  struct applicationState *stateptr = (void *)state;
  // stateptr->motorState = 1;
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
  // stateptr->motorMovementStartTime = time(NULL);
  // time(stateptr->motorMovementStartTime);
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

  switch (stateptr->motorProcessIdentifier) {
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
      collectUserInput((void *)stateptr);
      break;
    case 5:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = CCW_BUMP * BUMP_AMPLIFIER;
      break;
    case 6:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = (CW_BUMP - CCW_BUMP) * BUMP_AMPLIFIER;
      break;
    case 7:
    // to-test: modify the changeInAngularPosition to be calculated based off of the min and max values for position,
    // corrected back from steps to angular displacement
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      splitHardStops((void *)stateptr);
      // stateptr->changeInAngularPosition = -CW_BUMP;
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
  return NULL;
}


void *performanceManeuver(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 5;
  moveMotor((void *)stateptr);
  stateptr->hardStopMin = stateptr->currentPosition;
  stateptr->motorMovementStartTime = time(NULL); 
  stateptr->motorProcessIdentifier = 6;
  moveMotor((void *)stateptr);
  stateptr->hardStopMax = stateptr->currentPosition;
  stateptr->motorMovementStartTime = time(NULL);
  stateptr->motorProcessIdentifier = 7;
  moveMotor((void *)stateptr);
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
    default:
      break;
    }
  }
  return NULL;
}