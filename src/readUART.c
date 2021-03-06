#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "./serial.h"
#include "./narrative.h"
#include "./readUART.h"
#include "../inc/config.h"

static uint32_t sequence = 0x00;
static uint32_t encoder = 0x00;
static uint32_t index = 0x00;
static uint32_t temp = 2;
static uint8_t count = 0;


uint32_t correctFeedback(uint32_t encoderData, int processID) {
  if (encoderData > 0x12000) {
    encoderData *= (-1);
  }
  return encoderData;
}

uint32_t compareIndices(uint32_t previousIndex, uint32_t currentIndex) {
  uint32_t indexDifference = 0;
  if ((previousIndex < 0 && currentIndex > 0) || (previousIndex > 0 && currentIndex < 0)) {
    indexDifference = abs(currentIndex) + abs(previousIndex);
  } else {
    indexDifference = abs(currentIndex - previousIndex);
  }
  return indexDifference;
}

#define SYNC_SIGNAL 0xA5CAFEA5

// Reads bytes from the serial port.
// Returns after all the desired bytes have been read, or if there is a
// timeout or other error.
// Returns the number of bytes successfully read into the buffer, or -1 if
// there was an error reading.
ssize_t readPort(struct applicationState *stateptr, uint8_t * buffer, size_t size, FILE * telemetry_csv) {
  size_t received = 0;

  while (received < size)
  {
    ssize_t r = read(*stateptr->fd, buffer + received, size - received);
    if (r < 0)
    {
      perror("failed to read from port");
      return -1;
    }
    if (r != 0) {
      switch (sequence) {
        case SYNC_SIGNAL:
          switch (count) {
            case 0:
              encoder += (uint32_t)buffer[0];
              count += 1;
              break;
            case 1:
            case 2:
              encoder = encoder<<8;
              encoder += (uint32_t)buffer[0];
              count += 1;
              break;
            case 3:
              encoder = encoder<<8;
              encoder += (uint32_t)buffer[0];
              if (stateptr->currentPosition != 200000) {
                temp = stateptr->currentPosition;
              }
              stateptr->currentPosition = encoder;
              stateptr->lastPosition = temp;
              temp = 100000;
              fprintf(telemetry_csv, "%17s,%8d,", stateptr->currentTime, encoder);
              fflush(telemetry_csv);
              encoder = 0x00;
              count += 1;
              break;
            case 4:
              index += (uint32_t)buffer[0];
              count += 1;
              break;
            case 5:
            case 6:
              index = index<<8;
              index += (uint32_t)buffer[0];
              count += 1;
              break;
            case 7:
              index = index<<8;
              index += (uint32_t)buffer[0];
              if (stateptr->currentIndex != 200000) {
                temp = stateptr->currentIndex;
              }
              stateptr->currentIndex = index;
              stateptr->lastIndex = temp;
              temp = 100000;
              fprintf(telemetry_csv, "%8d,%8d,%3d,%6f,%8d,%4d,%6d\n", 
                index, 
                compareIndices(
                  correctFeedback(stateptr->lastIndex, 0), 
                  correctFeedback(stateptr->currentIndex, 0)
                ),
                stateptr->directionOfRotation,
                stateptr->changeInAngularPosition,
                abs(stateptr->numberOfSteps),
                stateptr->performanceCycleCount,
                stateptr->cycleCount
              );
              fflush(telemetry_csv);
              index = 0x00;
              sequence = 0x00;
              count = 0;
              break;
            default:
              break;
          }
          break;
        default:
          sequence = sequence<<8;
          sequence += (uint32_t)buffer[0];
          break;
      }
      received += r;
      break;
    } 
  }
  return received;
}

int closeFile(FILE * telemetry_csv) {
  fflush(telemetry_csv);
  fclose(telemetry_csv);
  return 0;
}

void *readEncoderFeedback(void *state) {
  struct applicationState *stateptr = (void *)state;
  pthread_mutex_t lock;

  struct tm *timenow;
  time_t now = time(NULL);
  timenow = gmtime(&now);
  strftime(stateptr->filename, sizeof(stateptr->filename), "data/%Y%m%d_%H%M%S_DATA.csv", timenow);

  stateptr->fp = fopen(stateptr->filename, "w+");

  fprintf(stateptr->fp, "TIMESTAMP,POSITION_RAW,INDEX_RAW,INDEX_DELTA,DIRECTION_OF_ROTATION,CHANGE_IN_POSITION,CALCULATED_STEPS,PERFORMANCE_CYCLE_COUNT,FUNCTIONAL_CYCLE_COUNT\n");
  fflush(stateptr->fp);

  for (;;) {
  uint8_t buffer[4] = {0x00};
    switch(stateptr->readState) {
      case 1:
        pthread_mutex_lock(&lock);
        readPort(stateptr, buffer, 1, stateptr->fp);
        pthread_mutex_unlock(&lock);
        break;
      case 0:
        closeFile(stateptr->fp);
        return NULL;
    }
  } 
}
