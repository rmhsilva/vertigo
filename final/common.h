#ifndef COMMON_H
#define COMMON_H

#define PC_TX       p9      // Rx / Tx pins for PC (ftdi) comms
#define PC_RX       p10

#include "mbed.h"

extern Serial ftdi;

void log_pc(char *msg);

#endif