#include "RF24.h"


void interrupt_delay(RF24 *radio) {
	radio->stopListening();
	for(uint32_t i=0; i<130;i++){
		__asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
	}
}
