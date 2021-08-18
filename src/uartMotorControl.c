#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "../inc/config.h"
#include "./serial.h"
#include "./readUART.h"
#include "./writeUART.h"
#include "./narrative.h"

pthread_mutex_t lock;

void *collectUserInput(void *state) {
  struct applicationState *stateptr = (void *)state;
  printf("\nPlease enter the desired angular velocity in degrees per second\n");
  scanf("%d", &stateptr->desiredOutRate);
  printf("\nHow far do you want to rotate (in degrees)?\n");
  scanf("%f", &stateptr->changeInAngularPosition);
  return 0;
}


int reset_command_values(uint8_t * command) {
  for(int i = 2; i < sizeof(command); i++) {
    command[i] = 0;
  }
  return 0;
}


void *resetEncoder(void *state) {
  struct applicationState *stateptr = (void *) state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 1;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *encoderOnline(void *state) {
  struct applicationState *stateptr = (void *)state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 3;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *powerOnMotor(void *state) {
  struct applicationState *stateptr = (void *)state;
  stateptr->motorState = 1;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 7;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *loadStepDirection(void *state) {
  struct applicationState *stateptr = (void *)state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 7;
  stateptr->loadStep = 0;
  generateUartCommand((void *)stateptr);
  writePort(*stateptr->fd, stateptr->commandSequence, sizeof(stateptr->commandSequence));
  return NULL;
}


void *loadMovementCommand(void *state) {
  struct applicationState *stateptr = (void *)state;
  reset_command_values(stateptr->commandSequence);
  stateptr->state = 7;
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

  switch (stateptr->motorProcessIdentifier) {
    case 1:
      stateptr->desiredOutRate = RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = MWIR_DROT;
      break;
    case 2:
      stateptr->desiredOutRate = RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = LWIR_DROT - MWIR_DROT;
      break;
    case 3:
      stateptr->desiredOutRate = RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = -LWIR_DROT;
      break;
    case 4:
      collectUserInput((void *)stateptr);
      break;
    case 5:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = CCW_BUMP;
      break;
    case 6:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = CW_BUMP - CCW_BUMP;
      break;
    case 7:
      stateptr->desiredOutRate = PERFORMANCE_RATE_OF_ROTATION;
      stateptr->changeInAngularPosition = -CW_BUMP;
      break;
    default:
      printf("Invalid movement process identifier: %d is undefined", stateptr->motorProcessIdentifier);
      break;
  }

  stateptr->delayCountUsec = abs((int)(((stateptr->changeInAngularPosition / stateptr->desiredOutRate) * 1.05) * 1000 * 1000));

  // encoderOnline((void *)stateptr);
  // delay(100000);
  powerOnMotor((void *)stateptr);
  delay(100000);
  loadStepDirection((void *)stateptr);
  delay(100000);
  loadMovementCommand((void *)stateptr);
  delay(100000);
  fprintf(stateptr->fp, "\n\nMOVEMENT\nDirection of Rotation: %d\nChange in Position: %f\n\n\n", stateptr->directionOfRotation, stateptr->changeInAngularPosition);
  fflush(stateptr->fp);
  // delay(stateptr->delayCountUsec);
  usleep((stateptr->delayCountUsec) + 5000000);
  stateptr->motorState = 0;

  
  pthread_mutex_unlock(&lock);
  return NULL;

}
