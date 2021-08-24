#ifndef UART_MOTOR_CONTROL
#define UART_MOTOR_CONTROL
#include <stdint.h>

  // int collectUserInput(int *desiredOutRate, float *changeInAngularPosition);
  // int reset_command_values(uint8_t * command);
  void *moveMotor(void *state);
  void *encoderOnline(void *state);
  void *resetEncoder(void *state);
  void *performanceManeuver(void *state);
  void *functionalManeuver(void *state);
  void *motorMoveMonitor(void *state);
  // int moveBookend(int *fd);
  // int activateMotor(int *fd);

#endif