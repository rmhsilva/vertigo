#include "mbed.h"
#include "common.h"
#include "config.h"
#include "gps.h"

int CLK = 0;
eventConfig event[3];

void runEvent(int id){
    switch(event[id].action)
    {
        case 1:
        if(CLK%event[id].eValue == 0){ 
            // Measure GPS and send back data
            send_gps = 1;
        }
        break;
        case 2:
        if(CLK%event[id].eValue == 0){ 
                // Measure temp and send back data
            send_temp = 1;
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
    CLK++;
}

void runTrigger(int id){
    if(event[id].eValue){
        switch(event[id].event)
        {
            case 1:
                //int alt = 0;
                // read altitude
            if(alt > event[id].eValue){ runEvent(id); }
            break;
            case 2:
                //int time = 0;
                //read time
            if((hour*60+minute) > event[id].eValue){ runEvent(id); }
            break;
            case 3:
                //int pos = 0;
                // read pos
                //if(pos > event[id].eValue){ runEvent(id); }
            break;
            default:
            break;
        }
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
        while(count < 6)
        {
            if (fgets(line, 128, fp) == NULL) return 2;
            switch(count)
            {
                case 1:
                tempNo = atoi(line);
                GPS_update = tempNo;
                ftdi.printf("GPS_update: %d \r\n", tempNo);
                break;
                case 2:
                tempNo = atoi(line);
                Temp_update = tempNo;
                ftdi.printf("Temp_update: %d \r\n", tempNo);
                break;
                case 3:
                event[0].valid = atoi(strtok(line, " "));
                event[0].event = atoi(strtok(NULL, " "));
                event[0].eValue = atoi(strtok(NULL, " "));
                event[0].action = atoi(strtok(NULL, " "));
                event[0].aValue = atoi(strtok(NULL, " "));
                ftdi.printf("event0 valid: %d, event: %d, eValue: %d, action: %d, aValue: %d \r\n", 
                    event[0].valid, event[0].event, event[0].eValue, event[0].action, event[0].aValue);
                break;
                case 4:
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