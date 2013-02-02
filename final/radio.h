#ifndef RADIO_H
#define RADIO_H

extern char buffer434[]; //Telem string buffer
extern char buffer868[]; //Telem string buffer

void rfm22_868_setup();
void rfm22_434_setup();

bool setFrequency_rfm868(float centre);
bool setFrequency_rfm434(float centre);
short CRC16_checksum (char *string);

void rtty_434_txstring(char * string);
void rtty_868_txstring(char * string);
void rtty_434_txbyte (char c);
void rtty_868_txbyte (char c);

void write_rfm868(char addr, char data);
void write_rfm868(char start_addr, char data[], char len);
char read_rfm868(char addr);
void read_rfm868(char start_addr, char buf[], char len);

char read_rfm434(char addr);
void read_rfm434(char start_addr, char buf[], char len);
void write_rfm434(char addr, char data);
void write_rfm434(char start_addr, char data[], char len);

#endif