#include "mbed.h"

DigitalOut led(LED1);
DigitalOut ss_port_434(p8);
SPI rfm434(p5, p6, p7);

char superbuffer [80]; //Telem string buffer

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
    rfm434.frequency(8000000); // 8MHz
    
    // vvv Leftover from old code
    // MSB first
    //SPI.setBitOrder(MSBFIRST);
}

void rtty_txbit (int bit)
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


void rtty_txbyte (char c)
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
    rtty_txbit (0); // Start bit
    // Send bits for for char LSB first    
    for (i=0;i<7;i++)
    {
        if (c & 1) rtty_txbit(1); 
            else rtty_txbit(0);    
        c = c >> 1;
    }
    rtty_txbit (1); // Stop bit
    rtty_txbit (1); // Stop bit
}

void rtty_txstring (char * string)
{

    /* Simple function to sent a char at a time to 
    ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    */
    char c;
    c = *string++;
    while ( c != '\0')
    {
        rtty_txbyte (c);
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



int main() {
	int n;
    led = 1;
    
    initSPI_434();

    init_rfm434();
    
    write_rfm434(0x71, 0x00); // unmodulated carrier
    //This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
    write_rfm434(0x0b,0x12);
    write_rfm434(0x0c,0x15);
  
    setFrequency_rfm434(434.201);
  
    write_rfm434(0x6D, 0x04);// turn tx low power 11db
  
    write_rfm434(0x07, 0x08); // turn tx on
    
    wait(5.0);
    n = sprintf(superbuffer, "$$MBED,HI,EVERYONE");
	n = sprintf(superbuffer, "%s*%04X\n", superbuffer, CRC16_checksum(superbuffer));
    rtty_txstring(superbuffer);
	
	while(1) {
		wait(3);
		n = sprintf(superbuffer, "$$MBED,HI,EVERYONE");
		n = sprintf(superbuffer, "%s*%04X\n", superbuffer, CRC16_checksum(superbuffer));
		rtty_txstring(superbuffer);
}
