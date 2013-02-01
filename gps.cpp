#include "mbed.h"
#include "gps.h"

// GPS constructor
GPS::GPS(PinName tx, PinName rx)
: gps(tx, rx)   // initialisation list... yes C++ is horrible
{
  uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
  this->gps.printf("%s", (char *)setNMEAoff);
}

// GET POSITION
int GPS::get_position()
{
  char position[60];
  int GPSerror = 0, i;

  // Request data
  uint8_t request[] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A, 0x0};
  this->gps.printf("%s", request);

  // getting data
  for(i=0; i<58 && this->gps.readable(); i++) {
    position[i] = this->gps.getc();
  }
  position[i] = 0x0;

  if( position[0] != 0xB5 ||position[1] != 0x62 )
    GPSerror = 21;
  if( position[2] != 0x01 || position[3] != 0x02 )
    GPSerror = 22;

  if( !this->verify_checksum((uint8_t *)&position[2], 32) ) {
    GPSerror = 23;
  }

  // GPS data
  if(GPSerror == 0) {
    // 4 bytes of longitude (1e-7)
    lon = (int32_t)position[10] | (int32_t)position[11] << 8 |
      (int32_t)position[12] << 16 | (int32_t)position[13] << 24;
    this->lon /= 1000;

    // 4 bytes of latitude (1e-7)
    lat = (int32_t)position[14] | (int32_t)position[15] << 8 |
      (int32_t)position[16] << 16 | (int32_t)position[17] << 24;
    this->lat /= 1000;

    // 4 bytes of altitude above MSL (mm)
    alt = (int32_t)position[22] | (int32_t)position[23] << 8 |
      (int32_t)position[24] << 16 | (int32_t)position[25] << 24;
    this->alt /= 1000;
  }

  return GPSerror;
}

// Time
int GPS::get_time()
{
  char time[60];
  int GPSerror = 0, i;

  uint8_t request[9] = {0xB5, 0x62, 0x01, 0x21, 0x00, 0x00, 0x22, 0x67, 0x0};
  this->gps.printf("%s", request);

  // getting data
  for(i=0; i<58 && this->gps.readable(); i++) {
    time[i] = this->gps.getc();
  }
  time[i] = 0x0;

  // Verify the sync and header bits
  if( time[0] != 0xB5 || time[1] != 0x62 )
    GPSerror = 31;
  if( time[2] != 0x01 || time[3] != 0x21 )
    GPSerror = 32;

  if( !this->verify_checksum((uint8_t *)&time[2], 24) ) {
    GPSerror = 33;
  }

  if(GPSerror == 0) {
    this->hour = time[22];
    this->minute = time[23];
    this->second = time[24];

    // Check for errors in the value
    if(hour > 23 || minute > 59 || second > 59)
    {
      GPSerror = 34;
    }
  }

  // Send error back
  return GPSerror;
}

char GPS::check_lock()
{
  int GPSerror = 0, i;
  char lock[60];
  char check;

  // Construct the request to the GPS
  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x06, 0x00, 0x00, 0x07, 0x16};
  this->gps.printf("%s", request);

  // getting data
  for(i=0; i<58 && this->gps.readable(); i++) {
    lock[i] = this->gps.getc();
  }
  lock[i] = 0x0;

  // Verify the sync and header bits
  if( lock[0] != 0xB5 || lock[1] != 0x62 ) {
    GPSerror = 11;
  }
  if( lock[2] != 0x01 || lock[3] != 0x06 ) {
    GPSerror = 12;
  }

  // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
  if( !this->verify_checksum((uint8_t *)&lock[2], 56) ) {
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

  //sats = buf[53];
  }
  else {
    check = 0;
  }

  return check;
}

bool GPS::verify_checksum(uint8_t* data, uint8_t len)
{
  uint8_t a, b;
  return true;
  //gps_ubx_checksum(data, len, &a, &b);
  if( a != *(data + len) || b != *(data + len + 1))
    return false;
  else
    return true;
}