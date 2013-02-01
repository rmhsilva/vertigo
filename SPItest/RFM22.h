
#ifndef rfm22_h
#define rfm22_h
#include "mbed.h"

DigitalOut ss_port_434(p8);
SPI rfm434(p5, p6, p7);

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

#endif
