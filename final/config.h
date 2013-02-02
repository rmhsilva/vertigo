#ifndef CONFIG_H
#define CONFIG_H

typedef struct  {
    int valid;
    int event;
    int eValue;
    int action;
    int aValue; 
} eventConfig;

extern int send_gps, send_temp;

int Config();
void runTrigger(int id);

#endif