#include "mbed.h"

Serial pc(USBTX, USBRX);

int main() {
	int i;

	while(1) {
		for (i=0; i<10; i++) {
			pc.printf("Hello, number %d", i);
		}
	}
	
	return 0;
}