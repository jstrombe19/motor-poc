#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "../inc/config.h"
#include "./writeUART.h"
#include "./serial.h"
#include "./narrative.h"

void delay(int numberOfMicroseconds) {
  int microseconds =  numberOfMicroseconds;
  clock_t start_time = clock();
  while(clock() < start_time + microseconds){};
}

int getDigit(int num, int n) {
  int r;
  r = num / pow(16, n);
  r = r % 16;
  return r;
}

void *convertInputToUartValues(void *state) { // invoke with 1 for Honeybee; invoke with 0 for sample engineering motor
    struct applicationState *stateptr = (void *)state;
    int ptsPerDDSPeriod                 = 1024; // fixed in FW
    int stepsPerPeriod                  = 6;     // defined by three-phase motor
    float ptsPerStep                    = ptsPerDDSPeriod * 1.0 / stepsPerPeriod;    // pts per step
    double delayPcnt                    = 1.00E-8; // fixed in FW
    int ptsPerPhaseIncrement            = 16; // defined in FW - mutable - phase step size
    int motorStepsPerRevolution         = 0;
    int gearRatio                       = 0;
    switch(MOTOR_TYPE) {
      case 0:
        motorStepsPerRevolution         = 300;
        gearRatio                       = 1; 
        break;
      case 1:
        motorStepsPerRevolution         = 24;
        gearRatio                       = 36;
        break;
    }
    
    float ptsPerRevolution              = ptsPerStep * motorStepsPerRevolution;
    int stepDelay                       = round((ptsPerPhaseIncrement * 360)/(delayPcnt * ptsPerRevolution * gearRatio * stateptr->desiredOutRate));
    int outputNumberStepsPerRevolution  = (ptsPerRevolution / ptsPerPhaseIncrement) * gearRatio;
    /*
      the necessary number of steps to produce any movement is calculated by first determining what percentage of a
      revolution is necessary - the change in angular position divided by 360 degrees - multiplied by the total number
      of steps available at the output shaft that equate to a single revolution
    */
    int numberOfSteps                   = abs(round(stateptr->changeInAngularPosition / 360.0 * outputNumberStepsPerRevolution));

    stateptr->stepDelay = be32toh(stepDelay);
    stateptr->numberOfSteps = be32toh(numberOfSteps);
    stateptr->phaseStepSize = be16toh(ptsPerPhaseIncrement);

    return NULL;
}


void *generateUartCommand(void *state) {
    struct applicationState *stateptr = (void *)state;
    if(stateptr->changeInAngularPosition < 0) {
      stateptr->directionOfRotation = 3;
    } else {
      stateptr->directionOfRotation = 1;
    }
    stateptr->commandSequence[2] = stateptr->state; // [2:0] {0x0,slpn,rstn,ena} # enable and take out of rst/sleep
    stateptr->commandSequence[3] = stateptr->directionOfRotation; // [1:0] motor mode, b1=fwdn b0=ld dir
    stateptr->commandSequence[4] = stateptr->loadStep; // [0:0] motor mode,         b0=ld step
    *(uint16_t*)&stateptr->commandSequence[5] = stateptr->phaseStepSize;
    *(uint32_t*)&stateptr->commandSequence[7] = stateptr->numberOfSteps;
    *(uint32_t*)&stateptr->commandSequence[11] = stateptr->stepDelay; // delay W.R.T. 100Mhz clocks
                                                  // 100uS/10nS = 10,000 = 0x2710

  return NULL;
}



// Writes bytes to the serial port, returning 0 on success and -1 on failure.
int writePort(int fd, uint8_t * write_buffer, size_t size)
{
  for(int i = 0; i < size; i++) {
    // printf("write_buffer[%d]: %x\n", i, write_buffer[i]);
  }
  ssize_t result = write(fd, write_buffer, size);
  delay(100000);
  if (result != (ssize_t)size)
  {
    perror("failed to write to port");
    return -1;
  }
  return 0;
}
