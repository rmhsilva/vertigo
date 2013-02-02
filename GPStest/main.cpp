#include "mbed.h"

/*****[ Defines, constants etc ]*****************************************/

#define PC_TX       p9      // Rx / Tx pins for PC (ftdi) comms
#define PC_RX       p10

#define GPS_TX      p28
#define GPS_RX      p27

#define TEMP_IN     p20       // Analogue temperature sensor in


/*****[ Declarations ]***************************************************/

// Set up serial port
Serial ftdi(PC_TX, PC_RX);

// GPS
Serial gps(GPS_TX, GPS_RX);

// RFM22 434
DigitalOut ss_port_434(p8);
SPI rfm434(p5, p6, p7);

// RFM22 868
DigitalOut ss_port_868(p14);
SPI rfm868(p11, p12, p13);

AnalogIn temp(TEMP_IN);

int GPSerror = 0, count = 0, lock=0, dgps=0, lat=0, lon=0, alt=0, sats=0, hour=0, minute=0, second=0;

int temperature;

char buffer434 [80]; //Telem string buffer

char buffer868 [160]; //Telem string buffer

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
  uint8_t gpsbuf[80];
  int check;
  Timer t;
  
  GPSerror = 0;

  // Construct the request to the GPS
  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x06, 0x00, 0x00, 0x07, 0x16};
  sendUBX(request, 8);
  //gps.printf("%s", request);

  // getting data
  t.reset();
  t.start();
  while (1) {
    if(gps.readable()) {
        gpsbuf[i] = gps.getc();
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
  if( gpsbuf[0] != 0xB5 || gpsbuf[1] != 0x62 ) {
    GPSerror = 11;
  }
  if( gpsbuf[2] != 0x01 || gpsbuf[3] != 0x06 ) {
    GPSerror = 12;
  }

  // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
  if( !gps_verify_checksum((uint8_t *)&gpsbuf[2], 56) ) {
    GPSerror = 13;
  }

  if(GPSerror == 0){
    // Return the value if GPSfixOK is set in 'flags'
    if( gpsbuf[17] & 0x01 ) {
    //check = buf[16];
      dgps = (gpsbuf[17] & 0x02);
      check = gpsbuf[16];
    } else
      check = 0;

  sats = gpsbuf[53];
  }
  else {
    check = 0;
  }

  lock = check;
  return GPSerror;
}

// GET POSITION
int gps_get_position()
{
  char gpsbuf[80];
  int i=0;
  Timer t;
  
  GPSerror = 0;

  // Request data
  uint8_t request[9] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A, 0x0};
  sendUBX(request, 9);

  // getting data
  t.reset();
  t.start();
  while (1) {
    if(gps.readable()) {
        gpsbuf[i] = gps.getc();
        //ftdi.printf("0x%x ",gpsbuf[i]);
        i++;
    }
    if(t.read_ms()>1000) {
        break;
    }
  }
  t.stop();

  if( gpsbuf[0] != 0xB5 || gpsbuf[1] != 0x62 )
    GPSerror = 21;
  if( gpsbuf[2] != 0x01 || gpsbuf[3] != 0x02 )
    GPSerror = 22;
  
  if( !gps_verify_checksum((uint8_t *)&gpsbuf[2], 32) ) {
    GPSerror = 23;
  }

  // GPS data
  if(GPSerror == 0) {
    // 4 bytes of longitude (1e-7)
    lon = (int32_t)gpsbuf[10] | (int32_t)gpsbuf[11] << 8 |
      (int32_t)gpsbuf[12] << 16 | (int32_t)gpsbuf[13] << 24;
    lon /= 1000;

    // 4 bytes of latitude (1e-7)
    lat = (int32_t)gpsbuf[14] | (int32_t)gpsbuf[15] << 8 |
      (int32_t)gpsbuf[16] << 16 | (int32_t)gpsbuf[17] << 24;
    lat /= 1000;

    // 4 bytes of altitude above MSL (mm)
    alt = (int32_t)gpsbuf[22] | (int32_t)gpsbuf[23] << 8 |
      (int32_t)gpsbuf[24] << 16 | (int32_t)gpsbuf[25] << 24;
    alt /= 1000;
  }

  return GPSerror;
}

int gps_get_time()
{
  char time[80];
  int i=0;
  Timer t;
  
  GPSerror = 0;
  uint8_t request[9] = {0xB5, 0x62, 0x01, 0x21, 0x00, 0x00, 0x22, 0x67, 0x0};
  sendUBX(request, 9);

  // getting data
  t.reset();
  t.start();
  while (1) {
    if(gps.readable()) {
        time[i] = gps.getc();
        //ftdi.printf("0x%x ",lock[i]);
        i++;
    }
    if(t.read_ms()>1000) {
        break;
        }
    }
    t.stop();

  // Verify the sync and header bits
  if( time[0] != 0xB5 || time[1] != 0x62 )
    GPSerror = 31;
  if( time[2] != 0x01 || time[3] != 0x21 )
    GPSerror = 32;

  if( !gps_verify_checksum((uint8_t *)&time[2], 24) ) {
    GPSerror = 33;
  }

  if(GPSerror == 0) {
    hour = time[22];
    minute = time[23];
    second = time[24];

    // Check for errors in the value
    if(hour > 23 || minute > 59 || second > 59)
    {
      GPSerror = 34;
    }
  }

  // Send error back
  return GPSerror;
}

char read_rfm434(char addr) {
    //write ss low to start
    ss_port_434 = 0;
    
    // make sure the msb is 0 so we do a read, not a write
    addr &= 0x7F;
    rfm434.write(addr);
    char val = rfm434.write(0x00);
    
    //write ss high to end
    ss_port_434 = 1;
    
    return val;
}

void write_rfm434(char addr, char data) {
    //write ss low to start
    ss_port_434 = 0;
    
    // make sure the msb is 1 so we do a write
    addr |= 0x80;
    rfm434.write(addr);
    rfm434.write(data);
    
    //write ss high to end
    ss_port_434 = 1;
}

void read_rfm434(char start_addr, char buf[], char len) {
    //write ss low to start
    ss_port_434 = 0;

    // make sure the msb is 0 so we do a read, not a write
    start_addr &= 0x7F;
    rfm434.write(start_addr);
    for (int i = 0; i < len; i++) {
        buf[i] = rfm434.write(0x00);
    }

    //write ss high to end
    ss_port_434 = 1;
}

void write_rfm434(char start_addr, char data[], char len) {
    //write ss low to start
    ss_port_434 = 0;

    // make sure the msb is 1 so we do a write
    start_addr |= 0x80;
    rfm434.write(start_addr);
    for (int i = 0; i < len; i++) {
        rfm434.write(data[i]);
    }

    //write ss high to end
    ss_port_434 = 1;
}

void resetFIFO_rfm434() {
    write_rfm434(0x08, 0x03);
    write_rfm434(0x08, 0x00);
}

void setInterrupt_rfm434(int interrupt, int is_on) {
    //high bits of interrupt are 0x03/0x05, low are 0x04/0x06
    char high = (interrupt >> 8) & 0xff;//    0x03/0x05
    char high_is_on = (is_on >> 8) & 0xff;
    char low = interrupt & 0xff;//        0x04/0x06
    char low_is_on = is_on & 0xff;
  
    // read out a set of regs
    //char regs = read(0x05);
    // mask out the values we will leave along
    //char ignore = regs & (~high);
    // zero out the values we don't care about (probably improperly set)
    //char important = high_is_on & high;
    // combine the two sets and write back out
    write_rfm434(0x05, (read_rfm434(0x05) & (~high)) | (high_is_on & high));
    
    write_rfm434(0x06, (read_rfm434(0x06) & (~low))  | (low_is_on     & low));
}

int readAndClearInterrupts_rfm434() {
    //high bits are 0x03, low are 0x04
    return (read_rfm434(0x03) << 8) | read_rfm434(0x04);
}

// Returns true if centre + (fhch * fhs) is within limits
// Caution, different versions of the RF22 suport different max freq
// so YMMV
bool setFrequency_rfm434(float centre)
{
    char fbsel = 0x40;
    if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
        return false;
    if (centre >= 480.0)
    {
        centre /= 2;
        fbsel |= 0x20;
    }
    centre /= 10.0;
    float integerPart = (int)centre;
    float fractionalPart = centre - integerPart;
    
    char fb = (char)integerPart - 24; // Range 0 to 23
    fbsel |= fb;
    int fc = fractionalPart * 64000;
    write_rfm434(0x73, 0);  // REVISIT
    write_rfm434(0x74, 0);
    write_rfm434(0x75, fbsel);
    write_rfm434(0x76, fc >> 8);
    write_rfm434(0x77, fc & 0xff);
    
    return true;
}

void init_rfm434() {
    // disable all interrupts
    write_rfm434(0x06, 0x00);
    
    // move to ready mode
    write_rfm434(0x07, 0x01);
    
    // set crystal oscillator cap to 12.5pf (but I don't know what this means)
    write_rfm434(0x09, 0x7f);
    
    // GPIO setup - not using any, like the example from sfi
    // Set GPIO clock output to 2MHz - this is probably not needed, since I am ignoring GPIO...
    write_rfm434(0x0A, 0x05);//default is 1MHz
    
    // GPIO 0-2 are ignored, leaving them at default
    write_rfm434(0x0B, 0x00);
    write_rfm434(0x0C, 0x00);
    write_rfm434(0x0D, 0x00);
    // no reading/writing to GPIO
    write_rfm434(0x0E, 0x00);
    
    // ADC and temp are off
    write_rfm434(0x0F, 0x70);
    write_rfm434(0x10, 0x00);
    write_rfm434(0x12, 0x00);
    write_rfm434(0x13, 0x00);
    
    // no whiting, no manchester encoding, data rate will be under 30kbps
    // subject to change - don't I want these features turned on?
    write_rfm434(0x70, 0x20);
    
    // RX Modem settings (not, apparently, IF Filter?)
    // filset= 0b0100 or 0b1101
    // fuck it, going with 3e-club.ru's settings
    write_rfm434(0x1C, 0x04);
    write_rfm434(0x1D, 0x40);//"battery voltage" my ass
    write_rfm434(0x1E, 0x08);//apparently my device's default
    
    // Clock recovery - straight from 3e-club.ru with no understanding
    write_rfm434(0x20, 0x41);
    write_rfm434(0x21, 0x60);
    write_rfm434(0x22, 0x27);
    write_rfm434(0x23, 0x52);
    // Clock recovery timing
    write_rfm434(0x24, 0x00);
    write_rfm434(0x25, 0x06);
    
    // Tx power to max
    write_rfm434(0x6D, 0x04);//or is it 0x03?
    
    // Tx data rate (1, 0) - these are the same in both examples
    write_rfm434(0x6E, 0x27);
    write_rfm434(0x6F, 0x52);
    
    // "Data Access Control"
    // Enable CRC
    // Enable "Packet TX Handling" (wrap up data in packets for bigger chunks, but more reliable delivery)
    // Enable "Packet RX Handling"
    write_rfm434(0x30, 0x8C);
    
    // "Header Control" - appears to be a sort of 'Who did i mean this message for'
    // we are opting for broadcast
    write_rfm434(0x32, 0xFF);
    
    // "Header 3, 2, 1, 0 used for head length, fixed packet length, synchronize word length 3, 2,"
    // Fixed packet length is off, meaning packet length is part of the data stream
    write_rfm434(0x33, 0x42);
    
    // "64 nibble = 32 byte preamble" - write_rfm434 this many sets of 1010 before starting real data. NOTE THE LACK OF '0x'
    write_rfm434(0x34, 64);
    // "0x35 need to detect 20bit preamble" - not sure why, but this needs to match the preceeding register
    write_rfm434(0x35, 0x20);
    
    // synchronize word - apparently we only set this once?
    write_rfm434(0x36, 0x2D);
    write_rfm434(0x37, 0xD4);
    write_rfm434(0x38, 0x00);
    write_rfm434(0x39, 0x00);
    
    // 4 bytes in header to send (note that these appear to go out backward?)
    write_rfm434(0x3A, 's');
    write_rfm434(0x3B, 'o');
    write_rfm434(0x3C, 'n');
    write_rfm434(0x3D, 'g');
    
    // Packets will have 1 bytes of real data
    write_rfm434(0x3E, 1);
    
    // 4 bytes in header to recieve and check
    write_rfm434(0x3F, 's');
    write_rfm434(0x40, 'o');
    write_rfm434(0x41, 'n');
    write_rfm434(0x42, 'g');
    
    // Check all bits of all 4 bytes of the check header
    write_rfm434(0x43, 0xFF);
    write_rfm434(0x44, 0xFF);
    write_rfm434(0x45, 0xFF);
    write_rfm434(0x46, 0xFF);
    
    //No channel hopping enabled
    write_rfm434(0x79, 0x00);
    write_rfm434(0x7A, 0x00);
    
    // FSK, fd[8]=0, no invert for TX/RX data, FIFO mode, no clock
    write_rfm434(0x71, 0x22);
    
    // "Frequency deviation setting to 45K=72*625"
    write_rfm434(0x72, 0x48);
    
    // "No Frequency Offet" - channels?
    write_rfm434(0x73, 0x00);
    write_rfm434(0x74, 0x00);
    
    // "frequency set to 434MHz" board default
    write_rfm434(0x75, 0x53);        
    write_rfm434(0x76, 0x64);
    write_rfm434(0x77, 0x00);
    
    resetFIFO_rfm434();
}

void initSPI_434() {

    ss_port_434 = 1;

    rfm434.format(8,0); // 8 bits per frame, mode 0
    rfm434.frequency(4000000); // 4MHz
    
    // vvv Leftover from old code
    // MSB first
    //SPI.setBitOrder(MSBFIRST);
}

char read_rfm868(char addr) {
    //write ss low to start
    ss_port_868 = 0;
    
    // make sure the msb is 0 so we do a read, not a write
    addr &= 0x7F;
    rfm868.write(addr);
    char val = rfm868.write(0x00);
    
    //write ss high to end
    ss_port_868 = 1;
    
    return val;
}

void write_rfm868(char addr, char data) {
    //write ss low to start
    ss_port_868 = 0;
    
    // make sure the msb is 1 so we do a write
    addr |= 0x80;
    rfm868.write(addr);
    rfm868.write(data);
    
    //write ss high to end
    ss_port_868 = 1;
}

void read_rfm868(char start_addr, char buf[], char len) {
    //write ss low to start
    ss_port_868 = 0;

    // make sure the msb is 0 so we do a read, not a write
    start_addr &= 0x7F;
    rfm868.write(start_addr);
    for (int i = 0; i < len; i++) {
        buf[i] = rfm868.write(0x00);
    }

    //write ss high to end
    ss_port_868 = 1;
}

void write_rfm868(char start_addr, char data[], char len) {
    //write ss low to start
    ss_port_868 = 0;

    // make sure the msb is 1 so we do a write
    start_addr |= 0x80;
    rfm868.write(start_addr);
    for (int i = 0; i < len; i++) {
        rfm868.write(data[i]);
    }

    //write ss high to end
    ss_port_868 = 1;
}

void resetFIFO_rfm868() {
    write_rfm868(0x08, 0x03);
    write_rfm868(0x08, 0x00);
}

void setInterrupt_rfm868(int interrupt, int is_on) {
    //high bits of interrupt are 0x03/0x05, low are 0x04/0x06
    char high = (interrupt >> 8) & 0xff;//    0x03/0x05
    char high_is_on = (is_on >> 8) & 0xff;
    char low = interrupt & 0xff;//        0x04/0x06
    char low_is_on = is_on & 0xff;
  
    // read out a set of regs
    //char regs = read(0x05);
    // mask out the values we will leave along
    //char ignore = regs & (~high);
    // zero out the values we don't care about (probably improperly set)
    //char important = high_is_on & high;
    // combine the two sets and write back out
    write_rfm868(0x05, (read_rfm868(0x05) & (~high)) | (high_is_on & high));
    
    write_rfm868(0x06, (read_rfm868(0x06) & (~low))  | (low_is_on     & low));
}

int readAndClearInterrupts_rfm868() {
    //high bits are 0x03, low are 0x04
    return (read_rfm868(0x03) << 8) | read_rfm868(0x04);
}

// Returns true if centre + (fhch * fhs) is within limits
// Caution, different versions of the RF22 suport different max freq
// so YMMV
bool setFrequency_rfm868(float centre)
{
    char fbsel = 0x40;
    if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
        return false;
    if (centre >= 480.0)
    {
        centre /= 2;
        fbsel |= 0x20;
    }
    centre /= 10.0;
    float integerPart = (int)centre;
    float fractionalPart = centre - integerPart;
    
    char fb = (char)integerPart - 24; // Range 0 to 23
    fbsel |= fb;
    int fc = fractionalPart * 64000;
    write_rfm868(0x73, 0);  // REVISIT
    write_rfm868(0x74, 0);
    write_rfm868(0x75, fbsel);
    write_rfm868(0x76, fc >> 8);
    write_rfm868(0x77, fc & 0xff);
    
    return true;
}

void init_rfm868() {
    // disable all interrupts
    write_rfm868(0x06, 0x00);
    
    // move to ready mode
    write_rfm868(0x07, 0x01);
    
    // set crystal oscillator cap to 12.5pf (but I don't know what this means)
    write_rfm868(0x09, 0x7f);
    
    // GPIO setup - not using any, like the example from sfi
    // Set GPIO clock output to 2MHz - this is probably not needed, since I am ignoring GPIO...
    write_rfm868(0x0A, 0x05);//default is 1MHz
    
    // GPIO 0-2 are ignored, leaving them at default
    write_rfm868(0x0B, 0x00);
    write_rfm868(0x0C, 0x00);
    write_rfm868(0x0D, 0x00);
    // no reading/writing to GPIO
    write_rfm868(0x0E, 0x00);
    
    // ADC and temp are off
    write_rfm868(0x0F, 0x70);
    write_rfm868(0x10, 0x00);
    write_rfm868(0x12, 0x00);
    write_rfm868(0x13, 0x00);
    
    // no whiting, no manchester encoding, data rate will be under 30kbps
    // subject to change - don't I want these features turned on?
    write_rfm868(0x70, 0x20);
    
    // RX Modem settings (not, apparently, IF Filter?)
    // filset= 0b0100 or 0b1101
    // fuck it, going with 3e-club.ru's settings
    write_rfm868(0x1C, 0x04);
    write_rfm868(0x1D, 0x40);//"battery voltage" my ass
    write_rfm868(0x1E, 0x08);//apparently my device's default
    
    // Clock recovery - straight from 3e-club.ru with no understanding
    write_rfm868(0x20, 0x41);
    write_rfm868(0x21, 0x60);
    write_rfm868(0x22, 0x27);
    write_rfm868(0x23, 0x52);
    // Clock recovery timing
    write_rfm868(0x24, 0x00);
    write_rfm868(0x25, 0x06);
    
    // Tx power to max
    write_rfm868(0x6D, 0x04);//or is it 0x03?
    
    // Tx data rate (1, 0) - these are the same in both examples
    write_rfm868(0x6E, 0x27);
    write_rfm868(0x6F, 0x52);
    
    // "Data Access Control"
    // Enable CRC
    // Enable "Packet TX Handling" (wrap up data in packets for bigger chunks, but more reliable delivery)
    // Enable "Packet RX Handling"
    write_rfm868(0x30, 0x8C);
    
    // "Header Control" - appears to be a sort of 'Who did i mean this message for'
    // we are opting for broadcast
    write_rfm868(0x32, 0xFF);
    
    // "Header 3, 2, 1, 0 used for head length, fixed packet length, synchronize word length 3, 2,"
    // Fixed packet length is off, meaning packet length is part of the data stream
    write_rfm868(0x33, 0x42);
    
    // "64 nibble = 32 byte preamble" - write_rfm868 this many sets of 1010 before starting real data. NOTE THE LACK OF '0x'
    write_rfm868(0x34, 64);
    // "0x35 need to detect 20bit preamble" - not sure why, but this needs to match the preceeding register
    write_rfm868(0x35, 0x20);
    
    // synchronize word - apparently we only set this once?
    write_rfm868(0x36, 0x2D);
    write_rfm868(0x37, 0xD4);
    write_rfm868(0x38, 0x00);
    write_rfm868(0x39, 0x00);
    
    // 4 bytes in header to send (note that these appear to go out backward?)
    write_rfm868(0x3A, 's');
    write_rfm868(0x3B, 'o');
    write_rfm868(0x3C, 'n');
    write_rfm868(0x3D, 'g');
    
    // Packets will have 1 bytes of real data
    write_rfm868(0x3E, 1);
    
    // 4 bytes in header to recieve and check
    write_rfm868(0x3F, 's');
    write_rfm868(0x40, 'o');
    write_rfm868(0x41, 'n');
    write_rfm868(0x42, 'g');
    
    // Check all bits of all 4 bytes of the check header
    write_rfm868(0x43, 0xFF);
    write_rfm868(0x44, 0xFF);
    write_rfm868(0x45, 0xFF);
    write_rfm868(0x46, 0xFF);
    
    //No channel hopping enabled
    write_rfm868(0x79, 0x00);
    write_rfm868(0x7A, 0x00);
    
    // FSK, fd[8]=0, no invert for TX/RX data, FIFO mode, no clock
    write_rfm868(0x71, 0x22);
    
    // "Frequency deviation setting to 45K=72*625"
    write_rfm868(0x72, 0x48);
    
    // "No Frequency Offet" - channels?
    write_rfm868(0x73, 0x00);
    write_rfm868(0x74, 0x00);
    
    // "frequency set to 868MHz" board default
    write_rfm868(0x75, 0x53);        
    write_rfm868(0x76, 0x64);
    write_rfm868(0x77, 0x00);
    
    resetFIFO_rfm868();
}

void initSPI_868() {

    ss_port_868 = 1;

    rfm868.format(8,0); // 8 bits per frame, mode 0
    rfm868.frequency(4000000); // 4MHz
    
    // vvv Leftover from old code
    // MSB first
    //SPI.setBitOrder(MSBFIRST);
}

void rtty_434_txbit (int bit)
{
        if (bit)
        {
          // high
                  write_rfm434(0x73, 0x03);
        }
        else
        {
          // low
                  write_rfm434(0x73, 0x00);
        }
                     // 50 Baud:
                wait_us(19500); // 10000 = 100 BAUD 20150
                // Theoretical 100 Baud:
                //wait_us(9500); // (20000)/2 = 10000 - 500 as that seems to be how the one above is calculated
                // Theoretical 300 Baud:
                //wait_us(2850); // (20000)/6 = 3333.3 - 500 as that seems to be how the one above is calculated

}


void rtty_434_txbyte (char c)
{
    /* Simple function to sent each bit of a char to 
    ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    ** ASCII_BIT = 7 or 8 for ASCII-7 / ASCII-8
    */
    int i;
    rtty_434_txbit (0); // Start bit
    // Send bits for for char LSB first    
    for (i=0;i<7;i++)
    {
        if (c & 1) rtty_434_txbit(1); 
            else rtty_434_txbit(0);    
        c = c >> 1;
    }
    rtty_434_txbit (1); // Stop bit
    rtty_434_txbit (1); // Stop bit
}

void rtty_434_txstring (char * string)
{

    /* Simple function to sent a char at a time to 
    ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    */
    char c;
    c = *string++;
    while ( c != '\0')
    {
        rtty_434_txbyte (c);
        c = *string++;
    }
}

void rtty_868_txbit (int bit)
{
        if (bit)
        {
          // high
                  write_rfm868(0x73, 0x03);
        }
        else
        {
          // low
                  write_rfm868(0x73, 0x00);
        }
                     // 50 Baud:
                //wait_us(19500); // 10000 = 100 BAUD 20150
                // Theoretical 100 Baud:
                //wait_us(9500); // (20000)/2 = 10000 - 500 as that seems to be how the one above is calculated
                // Theoretical 300 Baud:
                //wait_us(3250); // (20000)/6 = 3333.3 - 500 as that seems to be how the one above is calculated
                wait_us(1550); // 600 BAUD!
}


void rtty_868_txbyte (char c)
{
    /* Simple function to sent each bit of a char to 
    ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    ** ASCII_BIT = 7 or 8 for ASCII-7 / ASCII-8
    */
    int i;
    rtty_868_txbit (0); // Start bit
    // Send bits for for char LSB first    
    for (i=0;i<7;i++)
    {
        if (c & 1) rtty_868_txbit(1); 
            else rtty_868_txbit(0);    
        c = c >> 1;
    }
    rtty_868_txbit (1); // Stop bit
    rtty_868_txbit (1); // Stop bit
}

void rtty_868_txstring (char * string)
{

    /* Simple function to sent a char at a time to 
    ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    */
    char c;
    c = *string++;
    while ( c != '\0')
    {
        rtty_868_txbyte (c);
        c = *string++;
    }
}

int crc_xmodem_update (int crc, char data)
    {
        int i;

        crc = crc ^ ((int)data << 8);
        for (i=0; i<8; i++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }

        return crc;
    }

short CRC16_checksum (char *string)
{
    int i;
    int crc;
    char c;
 
    crc = 0xFFFF;
 
    // Calculate checksum ignoring the first two $s
    for (i = 2; i < strlen(string); i++)
    {
        c = string[i];
        crc = crc_xmodem_update (crc, c);
    }
 
    return crc;
}

void rfm22_434_setup() {
    initSPI_434();

    init_rfm434();
    
    write_rfm434(0x71, 0x00); // unmodulated carrier
    //This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
    write_rfm434(0x0b,0x12);
    write_rfm434(0x0c,0x15);
}

void rfm22_868_setup() {
    initSPI_868();

    init_rfm868();
    
    write_rfm868(0x71, 0x00); // unmodulated carrier
    //This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
    write_rfm868(0x0b,0x12);
    write_rfm868(0x0c,0x15);
}

int main() {
    int n;
	int rssi_1, rssi_2;
    temperature = (int16_t)((temp.read_u16()-9930)/199.0);
    ftdi.printf("Initiliasing GPS..\r\n");
    gps_setup();
    ftdi.printf("Initiliasing RFM22..\r\n");
    //rfm22_434_setup();
    rfm22_868_setup();
    
    //setFrequency_rfm434(434.201);
    //write_rfm434(0x6D, 0x04);// turn tx low power 11db
    //write_rfm434(0x07, 0x08); // turn tx on
    
    setFrequency_rfm868(869.5);
    write_rfm868(0x6D, 0x04);// turn tx low power 11db
    write_rfm868(0x07, 0x08); // turn tx on
    
    ftdi.printf("Entering Loop.\r\n");
    for (;;) {
        // Retrieve new GPS values if required
        /* */

        //gps.get_position();
        //gps.get_time();
        
        
        // Print out the vals
        gps_check_lock();
        
        //ftdi.printf("lock:%d Error:%d\r\n",lock,GPSerror);
        
        if(gps_get_position()!=0) {
            gps_get_position(); // Try again if it failed
            }
        
        //ftdi.printf("lat:%d lon:%d alt:%d sats:%d Error:%d\r\n",lat,lon,alt,sats,GPSerror);
        
        gps_get_time();
        
        //ftdi.printf("time: %d:%d:%d Error:%d\r\n",hour,minute,second,GPSerror);
        
        //gps.scanf("%s", &gpsout);
        //ftdi.printf("Temp: 0x%x \r\n", temperature);
        
        n = sprintf (buffer434, "$$VERTIGO,%d,%02d%02d%02d,%ld,%ld,%ld,%d", count, hour, minute, second, lat, lon, alt, sats);
        n = sprintf (buffer434, "%s*%04X\n", buffer434, (CRC16_checksum(buffer434) & 0xFFFF));
        
		write_rfm868(0x07, 0x01); // turn tx off
		rssi_1 = ((int)read_rfm868(26)*51 - 12400)/100; // returned in dBm
		wait(0.5);
		rssi_2 = ((int)read_rfm868(26)*51 - 12400)/100;
		write_rfm868(0x07, 0x08); // turn tx on
		
        //temperature = (int16_t)((temp.read_u16()-9930)/199.0); // Radios need to be turned off here
        n = sprintf (buffer868, "$$8VERTIGO,%d,%02d%02d%02d,%ld,%ld,%ld,%d,%d,%d,%d", count, hour, minute, second, lat, lon, alt, sats, temperature, rssi_1, rssi_2);
        n = sprintf (buffer868, "%s*%04X\n", buffer868, (CRC16_checksum(buffer868) & 0xFFFF));
        //rtty_434_txstring(buffer434);
        rtty_868_txstring(buffer868);
        //ftdi.printf(superbuffer);

        count++;
        //statusLED = !statusLED;
    }

    return 0;
}