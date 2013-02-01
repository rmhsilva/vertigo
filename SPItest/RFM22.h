
#ifndef rfm22_h
#define rfm22_h
#include <SPI.h>

#define RFM_INT_FFERR		(1 << 16)
#define RFM_INT_TXFFAFULL	(1 << 14)
#define RFM_INT_XTFFAEM		(1 << 13)
#define RFM_INT_RXFFAFULL	(1 << 12)
#define RFM_INT_EXT		(1 << 11)
#define RFM_INT_PKSENT		(1 << 10)
#define RFM_INT_PKVALID		(1 << 9)
#define RFM_INT_CRERROR		(1 << 8)

#define RFM_INT_SWDET		(1 << 7)
#define RFM_INT_PREAVAL		(1 << 6)
#define RFM_INT_PREAINVAL	(1 << 5)
#define RFM_INT_RSSI		(1 << 4)
#define RFM_INT_WUT		(1 << 3)
#define RFM_INT_LBD		(1 << 2)
#define RFM_INT_CHIPRDY		(1 << 1)
#define RFM_INT_POR		(1)

class rfm22
{

	DigitalOut ss_port();
	SPI rfm;
	
public:
	rfm22(PinName mosi_pin, PinName miso_pin, PinName sclk_pin, PinName ss_pin)
	{
		mosipin = mosi_pin;
		misopin = miso_pin;
		sclkpin = sclk_pin;
		sspin = ss_pin;
		DigitalOut ss_port(ss_pin);
		ss_port = 1;
		SPI rfm(mosi_pin, miso_pin, sclk_pin);
	}
	
	uint8_t read(uint8_t addr) const;
	void write(uint8_t addr, uint8_t data) const;
	
	void read(uint8_t start_addr, uint8_t buf[], uint8_t len);
	void write(uint8_t start_addr, uint8_t data[], uint8_t len);

	void setInterrupt(uint16_t interrupt, uint16_t isOn);
	uint16_t readAndClearInterrupts();
	void resetFIFO();
	
	boolean setFrequency(float centre);
	void init();
	
	static void initSPI();
};

#endif
