#include "mbed.h"
#include "common.h"

// Set up serial port
Serial ftdi(PC_TX, PC_RX);

void log_pc(char *msg) {
    ftdi.printf("%s", msg);
}