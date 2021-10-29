#ifndef PACKET_UTILS
#define PACKET_UTILS
#include <stdint.h>


uint64_t getRawDatau64FromOffset(const uint8_t array[], uint8_t offset);

void setRawDatau64AtOffset(uint64_t raw_data, uint8_t array[], uint8_t offset);

#endif