#include "mbed.h"

/*****[ Defines, constants etc ]*****************************************/

#define PC_TX       p9      // Rx / Tx pins for PC (ftdi) comms
#define PC_RX       p10

#define GPS_TX      p13
#define GPS_RX      p14

#define TEMP_IN     p20       // Analogue temperature sensor in


/*****[ Declarations ]***************************************************/

// Set up serial port
Serial ftdi(PC_TX, PC_RX);

// GPS
Serial gps(GPS_TX, GPS_RX);

int GPSerror = 0, lock=0, lat=0, lon=0, alt=0, sats=0;

void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    gps.putc(MSG[i]);
  }
 }

void gps_setup() {
    uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
    sendUBX(setNMEAoff, sizeof(setNMEAoff)/sizeof(uint8_t));
}

/**
 * Calculate a UBX checksum using 8-bit Fletcher (RFC1145)
 */
void gps_ubx_checksum(uint8_t* data, uint8_t len, uint8_t* cka,
        uint8_t* ckb)
{
    *cka = 0;
    *ckb = 0;
    for( uint8_t i = 0; i < len; i++ )
    {
        *cka += *data;
        *ckb += *cka;
        data++;
    }
}

/**
 * Verify the checksum for the given data and length.
 */
bool gps_verify_checksum(uint8_t* data, uint8_t len)
{
    uint8_t a, b;
    gps_ubx_checksum(data, len, &a, &b);
    if( a != *(data + len) || b != *(data + len + 1))
        return false;
    else
        return true;
}

char gps_check_lock()
{
  int i=0;
  uint8_t lock[80];
  int check;
  Timer t;

  // Construct the request to the GPS
  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x06, 0x00, 0x00, 0x07, 0x16};
  sendUBX(request, 8);
  //gps.printf("%s", request);

  // getting data
  t.reset();
  t.start();
  while (1) {
    if(gps.readable()) {
        lock[i] = gps.getc();
        //ftdi.printf("0x%x ",lock[i]);
        i++;
    }
    if(t.read_ms()>1000) {
        break;
        }
    }
    t.stop();
  //lock[i] = 0x0;

  // Verify the sync and header bits
  if( lock[0] != 0xB5 || lock[1] != 0x62 ) {
    GPSerror = 11;
  }
  if( lock[2] != 0x01 || lock[3] != 0x06 ) {
    GPSerror = 12;
  }

  // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
  if( !gps_verify_checksum((uint8_t *)&lock[2], 56) ) {
    GPSerror = 13;
  }

  if(GPSerror == 0){
    // Return the value if GPSfixOK is set in 'flags'
    if( lock[17] & 0x01 ) {
    //check = buf[16];
    //dgps = buf[18];
      check = lock[16];
    } else
      check = 0;

  sats = lock[53];
  }
  else {
    check = 0;
  }

  return check;
}

// GET POSITION
int gps_get_position()
{
  char position[80];
  int i;
  Timer t;
  
  GPSerror = 0;

  // Request data
  uint8_t request[] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A, 0x0};
  sendUBX(request, 9);

  // getting data
  t.reset();
  t.start();
  while (1) {
    if(gps.readable()) {
        position[i] = gps.getc();
        //ftdi.printf("0x%x ",lock[i]);
        i++;
    }
    if(t.read_ms()>1000) {
        break;
        }
    }
    t.stop();

  if( position[0] != 0xB5 ||position[1] != 0x62 )
    GPSerror = 21;
  if( position[2] != 0x01 || position[3] != 0x02 )
    GPSerror = 22;

  if( !gps_verify_checksum((uint8_t *)&position[2], 32) ) {
    GPSerror = 23;
  }

  // GPS data
  if(GPSerror == 0) {
    // 4 bytes of longitude (1e-7)
    lon = (int32_t)position[10] | (int32_t)position[11] << 8 |
      (int32_t)position[12] << 16 | (int32_t)position[13] << 24;
    lon /= 1000;

    // 4 bytes of latitude (1e-7)
    lat = (int32_t)position[14] | (int32_t)position[15] << 8 |
      (int32_t)position[16] << 16 | (int32_t)position[17] << 24;
    lat /= 1000;

    // 4 bytes of altitude above MSL (mm)
    alt = (int32_t)position[22] | (int32_t)position[23] << 8 |
      (int32_t)position[24] << 16 | (int32_t)position[25] << 24;
    alt /= 1000;
  }

  return GPSerror;
}

int main() {    
    gps_setup();

    for (;;) {

        // Retrieve new GPS values if required
        /* */

        //gps.get_position();
        //gps.get_time();
        
        
        // Print out the vals
        lock = gps_check_lock();
        
        ftdi.printf("lock:%d, Error:%d, lat:%x, lon:%x, alt:%x, min:, done.\r\n",
                                                                    lock,
                                                                    GPSerror,
                                                                    0,//gps.lat,//gps_pos->lat,
                                                                    0,//gps.lon,//gps_pos->lon,
                                                                    0,//gps.alt,//gps_pos->alt,
                                                                    0);//gps.minute);//gps_t->minute);
        
        gps_get_position();
        ftdi.printf("lat:%d,lon:%d,alt:%d,sats:%d,Error:%d\r\n",lat,lon,alt,sats,GPSerror);
        //gps.scanf("%s", &gpsout);
        //ftdi.printf("Temp: 0x%x \r\n", temperature);
        
        wait(1.0);
        //statusLED = !statusLED;
    }

    return 0;
}