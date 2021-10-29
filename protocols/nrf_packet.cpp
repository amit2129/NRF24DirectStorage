#include "nrf_packet.h"
#include <iostream>
#include <stdint.h>


uint8_t NRFPacket::isResponse() const {
	return data[0] & 1;
}

uint8_t NRFPacket::getProtocol() const {
	return data[0] >> 5 & 0b111;
}

uint8_t NRFPacket::getOperation() const {
	return data[0] >> 1 & 0b1111;
}


void NRFPacket::setIsResponse(uint8_t isResponse) {
	data[0] &= ~1;
	data[0] |= isResponse;
}

void NRFPacket::setProtocol(uint8_t proto){
	data[0] &= ~(0b111 << 5);
	data[0] |= proto << 5;
}

void NRFPacket::setOperation(uint8_t op) {
	data[0] &= ~(0xF << 1);
	data[0] |= op << 1;
}


std::ostream & NRFPacket::output(std::ostream &os) const {
	os << "Packet: {\n";
	os << "\tprotocol:\t" << this->getProtocolName() << std::endl;
	os << "\tisResponse:\t" << (this->isResponse() ? "Yes" : "No") << std::endl;
	return os;
}