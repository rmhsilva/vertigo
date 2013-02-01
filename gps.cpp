// ------ GPS CODE
int GPSerror = 0, long, lat, alt, hour, minute, second, total_time;;

// GPS SETUP
void GPS_setup()
{
	uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
	gps.printf("%s", (char *)setNMEAoff);
}

// GET POSITION
void gps_get_position()
{
	bool GPSerror = false;
	char position[60]
	
	// Request data
	uint8_t request[8] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A};
	gps.printf("%s", request);
	
	// getting data
	gps.scanf("%s", &position);
	
	if( position[0] != 0xB5 ||position[1] != 0x62 )
        GPSerror = 21;
    if( position[2] != 0x01 || position[3] != 0x02 )
        GPSerror = 22;
	
	if( !_gps_verify_checksum(&position[2], 32) ) {
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
}

void gps_get_time()
{
	bool GPSerror = false;
	char time[60];

    uint8_t request[8] = {0xB5, 0x62, 0x01, 0x21, 0x00, 0x00, 0x22, 0x67, 0x0};
    gps.printf("%s", request);

    // getting data
	gps.scanf("%s", &time);

    // Verify the sync and header bits
    if( time[0] != 0xB5 || time[1] != 0x62 )
        GPSerror = 31;
    if( time[2] != 0x01 || time[3] != 0x21 )
        GPSerror = 32;

    if( !_gps_verify_checksum(&time[2], 24) ) {
      GPSerror = 33;
    }
    
    if(GPSerror == 0) {
      if(hour > 23 || minute > 59 || second > 59)
      {
        GPSerror = 34;
      }
      else {
        hour = time[22];
        minute = time[23];
        second = time[24];
        total_time = hour + minute + second;
      }
    }
}

bool _gps_verify_checksum(uint8_t* data, uint8_t len)
{
    uint8_t a, b;
    gps_ubx_checksum(data, len, &a, &b);
    if( a != *(data + len) || b != *(data + len + 1))
        return false;
    else
        return true;
}