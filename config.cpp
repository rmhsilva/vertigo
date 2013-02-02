#include "mbed.h"

DigitalOut myled(LED1);

typedef struct  {
    int valid;
    int event;
    int eValue;
    int action;
    int aValue; 
} eventConfig;

int CLK;

eventConfig event[3];

// Function Declarations
void runTrigger(int id);
void runEvent(int id);
int Config( void );

int main() {

    Config();

    while(1) {
        for(int i = 0; i < 3 ; i ++)
        { 
            runTrigger(i);
        }
    
    }    
}

void runTrigger(int id){
    if(event[id].eValue){
        switch(event[id].event)
        {
            case 1:
                int alt = 0;
                // read altitude
                 if(alt > event[id].eValue){ runEvent(id); }
                break;
            case 2:
                int time = 0;
                //read time
                if(time > event[id].eValue){ runEvent(id); }
                break;
            case 3:
                int pos = 0;
                // read pos
                if(pos > event[id].eValue){ runEvent(id); }
                break;
            default:
                break;
        }
    }
}

void runEvent(int id){
    switch(event[id].action)
        {
            case 1:
                if(CLK%event[id].eValue == 0){ 
                // Measure GPS and send back data
                }
                break;
            case 2:
                if(CLK%event[id].eValue == 0){ 
                // Measure temp and send back data
                }             
                break;
            case 3:
                if(CLK%event[id].eValue == 0){ 
                // Measure picture and send back data
                }
                break;
            default:
                break;
        }
}

int Config( void) {
    int GPS_update;
    int Temp_update;
    int tempNo;
    int count = 1;

    LocalFileSystem local("local");
    FILE *fp = fopen("/local/config.txt", "r");
    char line [ 128 ];

    if (!fp) {
        return 1;
    }
    else{
        while(! feof(fp) | count > 5)
        {
        switch(count)
            {
            case 1:
                fscanf(fp, "%[^\n]", line);
                tempNo = atoi(line);
                GPS_update = tempNo;
                break;
            case 2:
                fscanf(fp, "%[^\n]", line);
                tempNo = atoi(line);
                Temp_update = tempNo;
                break;
            case 3:
                fscanf(fp, "%[^\n]", line);
                event[0].valid = atoi(strtok(line, " "));
                event[0].event = atoi(strtok(NULL, " "));
                event[0].eValue = atoi(strtok(NULL, " "));
                event[0].action = atoi(strtok(NULL, " "));
                event[0].aValue = atoi(strtok(NULL, " "));
                break;
            case 4:
                fscanf(fp, "%[^\n]", line);
                event[1].valid = atoi(strtok(line, " "));
                event[1].event = atoi(strtok(NULL, " "));
                event[1].eValue = atoi(strtok(NULL, " "));
                event[1].action = atoi(strtok(NULL, " "));
                event[1].aValue = atoi(strtok(NULL, " "));
                break;
            case 5:
                fscanf(fp, "%[^\n]", line);
                event[2].valid = atoi(strtok(line, " "));
                event[2].event = atoi(strtok(NULL, " "));
                event[2].eValue = atoi(strtok(NULL, " "));
                event[2].action = atoi(strtok(NULL, " "));
                event[2].aValue = atoi(strtok(NULL, " "));
                break;
            default:
                break;
            }
            
            count++;            
        }   
    }
    
    fclose(fp);
    return 0;   
}
