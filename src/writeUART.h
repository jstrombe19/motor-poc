#ifndef WRITE_UART
#define WRITE_UART
#include <stdint.h>

    void delay(int numberOfMicroseconds);
    void *convertInputToUartValues(void *state);
    int getDigit(int num, int n);
    void *generateUartCommand(void *state);
    int writePort(int fd, uint8_t * write_buffer, size_t size);


#endif