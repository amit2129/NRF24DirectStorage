#include "utils.h"
#include <stdint.h>


uint64_t getRawDatau64FromOffset(const uint8_t array[], uint8_t offset) {
	uint64_t ret = 0;
    for(int8_t i = 0; i < sizeof(uint64_t); i++){
        ret <<= 8;
        ret |= (uint64_t)array[i + offset];
    }
	return ret;
}

void setRawDatau64AtOffset(uint64_t raw_data, uint8_t array[], uint8_t offset) {
    for(int8_t i = 7; i >= 0; --i ){
    	array[i + offset] = raw_data & 0xFF;
        raw_data >>= 8;
    }
}