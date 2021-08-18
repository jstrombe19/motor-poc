#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include "../inc/config.h"
#include "./cycleone.h"
#include "./narrative.h"
#include "./serial.h"
#include "./readUART.h"
#include "./writeUART.h"
#include "./uartMotorControl.h"

WINDOW *win;
pthread_t cycleOneThread, cycleTwoThread, readUARTThread, writeDataThread, writeUARTThread;
static char fullStartTime[40];
struct timeValues {
    double starttimerawmsec;
    double starthourraw;
    int starthour;
    double startminraw;
    int startmin;
    double startsec;
    struct timeval before;
    struct timeval after;
    long start;
    long end;
  };

void finish(int sig)
{
  delwin(win);
  endwin();
  exit(sig);
}

void *cycleone(void *state)
{
  struct applicationState *stateptr = (void *)state;
  if (stateptr->currentDirection == 1)
  {
    stateptr->currentDirection = 3;
  }
  else if (stateptr->currentDirection == 3)
  {
    stateptr->currentDirection = 1;
  }
  return NULL;
}

const char *displayDROT(uint8_t *rawDROT)
{
  if (*rawDROT == 1)
  {
    return "CLOCKWISE";
  }
  else if (*rawDROT == 3)
  {
    return "COUNTERCLOCKWISE";
  }
  else
  {
    return "NONE";
  }
}

// void *updateStateLog(void *state) {
//   struct applicationState *stateptr = (void *)state;
//   stateptr->log[3] = stateptr->log[2];
//   stateptr->log[2] = stateptr->log[1];
//   stateptr->log[1] = stateptr->log[0];
//   stateptr->log[0] = *"";
//   return NULL;
// }

void makeDetail(struct applicationState *state, struct timeValues *timeVals)
{
  uint8_t row;
  uint8_t col;
  
  gettimeofday(&timeVals->after, NULL);
  timeVals->end = (long)timeVals->after.tv_sec * 1000 + (long)timeVals->after.tv_usec / 1000;
  double uptimerawmsec = (timeVals->end - timeVals->start) / 1000.0;
  double uphourraw = uptimerawmsec / 3600.0;
  int uphour = floor(uphourraw);
  double upminraw = (uphourraw - uphour) * 60.0;
  int upmin = floor(upminraw);
  double upsec = (upminraw - upmin) * 60.0;

  row = 1;
  col = 2;
  mvwprintw(win, row++, col, "Start time: %s", fullStartTime);
  mvwprintw(win, row++, col, "Uptime: %03d:%02d:%04.3f", uphour, upmin, upsec);
  row += 2;
  mvwprintw(win, row++, col, "Sync Status: %ld", 100);
  mvwprintw(win, row++, col, "UART Port: %8s @ %8d baud", PORT, BAUD_RATE);
  row++;
  mvwprintw(win, row++, col, "Motor Status: %2d", state->motorState);
  row += 2;
  mvwprintw(win, row++, col, "Select the desired process:");
  mvwprintw(win, row++, col, "[1] - Start logging data");
  mvwprintw(win, row++, col, "[2] - Adjust calibrator arm position");
  mvwprintw(win, row++, col, "[3] - Execute calibrator arm performance maneuver");
  mvwprintw(win, row++, col, "[4] - Execute calibrator arm function check");
  mvwprintw(win, row++, col, "[5] - ABORT CURRENT MANEUVER");
  mvwprintw(win, row++, col, "[6] - Complete test operations and exit");
  mvwprintw(win, row++, col, "[7] - Reset encoder position and index");
  row += 6;
  mvwprintw(win, row++, col, "Direction of Rotation: %18s", displayDROT(&state->currentDirection));
  mvwprintw(win, row++, col, "Start Encoder: %8d | Start Index: %8d", 0x01, 0x01);
  mvwprintw(win, row++, col, "Encoder: %8d (Delta %8d) | Index: %8d (Delta %8d", 0x01, 0x01, 0x01, 0x01);
  row += 2;
  mvwprintw(win, row++, col, "----------------------------------------------LOG---------------------------------------------");
  for(int i = 0; i < 4; i++) {
    if(strlen(&state->log[i]) != 0){
      mvwprintw(win, row++, col, "%s", state->log[i]);
    }
  }

  return;
}

void interface(struct applicationState *state, struct timeValues *timeVals)
{
  int ch;
  struct tm *timenow;
  time_t now = time(NULL);
  timenow = localtime(&now);
  strftime(fullStartTime, sizeof(fullStartTime), "%Y %m %d at %H %M %S", timenow);


  gettimeofday(&timeVals->before, NULL);
  timeVals->start = (long)timeVals->before.tv_sec * 1000 + (long)timeVals->before.tv_usec / 1000;
  timeVals->starttimerawmsec = timeVals->start / 1000.0;
  timeVals->starthourraw = timeVals->starttimerawmsec / 3600.0;
  timeVals->starthour = floor(timeVals->starthourraw);
  timeVals->startminraw = (timeVals->starthourraw - timeVals->starthour) * 60.0;
  timeVals->startmin = floor(timeVals->startminraw);
  timeVals->startsec = (timeVals->startminraw - timeVals->startmin) * 60.0;

  initscr();

  if (getenv("ESCDELAY") == NULL)
    ESCDELAY = 25;

  win = newwin(NUMROWS, NUMCOLS, 1, 2);
  box(win, 0, 0);
  wrefresh(win);

  noecho();
  keypad(win, TRUE);
  curs_set(0);
  nodelay(win, TRUE);

  while (state->applicationActive)
  {
    makeDetail(state, timeVals);

    ch = wgetch(win);
    switch (ch)
    {
    case '1':   // start logging data
      state->readState = 1;
      pthread_create(&readUARTThread, NULL, readEncoderFeedback, (void*)state);
      encoderOnline((void *)state);
      break;
    case '2':   // adjust calibrator arm position
      state->motorProcessIdentifier = 4;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      delay(state->movementDelay);
      pthread_join(writeUARTThread, NULL);
      break;
    case '3':   // execute calibrator arm performance maneuver 
      state->motorProcessIdentifier = 5;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      state->motorProcessIdentifier = 6;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      state->motorProcessIdentifier = 7;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      break;
    case '4':   // execute calibrator arm function check
    state->motorProcessIdentifier = 1;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      state->motorProcessIdentifier = 2;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      state->motorProcessIdentifier = 3;
      pthread_create(&writeUARTThread, NULL, moveMotor, (void *)state);
      while (state->motorState != 0) {}
      pthread_join(writeUARTThread, NULL);
      break;
    case '5':   // ABORT current maneuver
      encoderOnline((void *)state);
      break;
    case '6':   // complete test operations and exit
      state->readState = 0;
      finish(1);
      break;
    case'7':
      resetEncoder((void *)state);
      break;
    default:
      break;
    }
  }
}

int main()
{
  struct applicationState state = {
    .syncStatus = 0,
    .motorState = 0,
    .currentPosition = 0,
    .currentIndex = 0,
    .currentDirection = 0,
    .currentTime = 0,
    .startTime = 0,
    .motorMovementStartTime = 0,
    .movementDelay = 0,
    .applicationActive = 1,
    .writeState = 0,
    .readState = 0,
    .filename = "",
    .state = 0,
    .motorMode = 0,
    .loadStep = 0,
    .phaseStepSize = 0x0000,
    .numberOfSteps = 0x00000000,
    .stepDelay = 0x00000000,
    .commandSequence = {SYNC_BYTE, SYNC_BYTE}
  };

  struct timeValues timeVals;

  int fd = openSerialPort(DEVICE, BAUD_RATE);
    if(fd >= 0) {
      state.fd = &fd; 
      interface(&state, &timeVals);
    } else {
      printf("Failed to open serial port at %s, %d baud\n", DEVICE, BAUD_RATE);  
      return 1; 
    }

  return 0;
}