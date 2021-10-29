#ifndef DIRECT_STORAGE_PACKET
#define DIRECT_STORAGE_PACKET
#include <stdint.h>
#include <sstream>
#include <string.h>
#include "packets.h"
#include "../utils.h"
#define PACKET_SIZE 32
#define PACKET_BASE_OFFSET 1

enum file_op {file_exists, create_file, delete_file, append_data, get_file_stats, read_file, list_files, response};
static const char *file_op_str[] =
        { "File Exists", "Create File", "Delete File", "Append Data", "File Size", "Read File", "List Files", "Response"};


void perform_ds_action(PacketAction *action);

//void parse_ds_request(DirectStoragePacket *packet, uint8_t *should_deliver_response);






class DSPacket: public NRFPacket {
	public:
		DSPacket() {}
		DSPacket(file_op file_operation=file_exists, bool isResponse=0);
		DSPacket(char packet_data[32]);
		DSPacket(file_op file_operation, bool isResponse, const std::string string_data);
		DSPacket(file_op file_operation, bool isResponse, uint64_t raw_data);
		void setBytes(uint16_t total_bytes, uint16_t sequence_bytes, char *data);
		void getBytes(uint16_t *total_bytes, uint16_t *sequence_bytes) const;
		void setData(const std::string string_data);
		std::string getStringData(void) const;
		uint64_t getRawDatau64(void) const;
		void setRawDatau64(uint64_t raw_data);

		std::string getRawString(void) const;
		void getRawData(char packet_data[PACKET_SIZE]) const;

		uint8_t setRawData(char packet_data[], uint8_t data_len);

	private:
		uint8_t responder_action(void);
		uint8_t requester_action(void);
};



DSPacket::DSPacket(file_op file_operation, bool isResponse) {
	for (uint8_t i = 0; i < PACKET_SIZE; i++) { data[i] = 0;}
	data[0] = 1 << 5 | file_operation << 1 | isResponse;
}

DSPacket::DSPacket(char packet_data[PACKET_SIZE]) {
	for (uint8_t i = 0; i < PACKET_SIZE; i++) {
		data[i] = packet_data[i];
	}
}

DSPacket::DSPacket(file_op file_operation, bool isResponse, const std::string string_data) {
	for (uint8_t i = 0; i < PACKET_SIZE; i++) { data[i] = 0;}
	data[0] = 1 << 5 | file_operation << 1 | isResponse;
	strcpy((char *)(data + 1), string_data.c_str());
}

DSPacket::DSPacket(file_op file_operation, bool isResponse, uint64_t raw_data) {
	for (uint8_t i = 0; i < PACKET_SIZE; i++) { data[i] = 0;}
	data[0] = 1 << 5 | file_operation << 1 | isResponse;
	setRawDatau64(raw_data);
}


std::string DSPacket::getStringData(void) const {
	char *c_str =  (char *) data + 1;
	return std::string(c_str, strlen(c_str));
}

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

uint64_t DSPacket::getRawDatau64(void) const {
	return getRawDatau64FromOffset(data, 1);
}


void DSPacket::setRawDatau64(uint64_t raw_data) {
    setRawDatau64AtOffset(raw_data, data, 1);
}


void DSPacket::setData(std::string string_data) {
	char const *c = string_data.c_str();
	strcpy((char *)(data + 1), c);
}



inline std::ostream & operator<<(std::ostream & Str, DSPacket const & v);
std::ostream & operator<<(std::ostream & Str, DSPacket const & v) { 
	NRFPacket nrfPacket = v;
	Str << nrfPacket;
	Str << "\toperation:\t" << file_op_str[v.getOperation()] << std::endl;
	if (v.isResponse()) {
		if (v.getOperation() == get_file_stats) {
			Str << "\tfile_len:\t" << v.getRawDatau64() << std::endl;	
		}
		else {
			Str << "\tdata:\t\t" << v.getStringData() << std::endl;		
		}
		
	}
	else if (v.getOperation() == get_file_stats) {
		Str << "\tfilename:\t" << v.getStringData() << std::endl;	
	}

	Str << "}\n" << std::endl;
	
	return Str;
}

std::string DSPacket::getRawString() const {
  	std::ostringstream os;
	for (int i = 0; i < 32; i++) {
		if (i % 4 == 0)
			os << std::hex << "\n0x" << std::hex; 
		os << std::hex << (uint64_t) this->data[i]; 
	}
	os << std::endl;
	return os.str();
}


void DSPacket::getRawData(char packet_data[PACKET_SIZE]) const {
	for (uint8_t i = 0; i < PACKET_SIZE; i++) {
		packet_data[i] = data[i];
	}
}

uint8_t DSPacket::setRawData(char packet_data[], uint8_t data_len) {
	if (data_len > PACKET_SIZE) {
		return 1;
	}
	while (data_len--) {
		*(data + 1 + data_len) = packet_data[data_len];
	}
	return 0;
}


#endif