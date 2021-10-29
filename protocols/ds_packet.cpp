#ifndef DS_PACKET
#define DS_PACKET


#include <stdint.h>
#include <sstream>
#include <string.h>
#include "../utils.h"
#include "nrf_packet.h"

#include "ds_packet.h"
#include "utils.h"


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


std::ostream & DSPacket::output(std::ostream &os) const {
	os << "Packet: {\n";
	os << "\tprotocol:\t" << this->getProtocolName() << std::endl;
	os << "\tisResponse:\t" << (this->isResponse() ? "Yes" : "No") << std::endl;
	os << "\toperation:\t" << file_op_str[this->getOperation()] << std::endl;
	if (this->isResponse()) {
		switch (this->getOperation()) {
			case file_exists:
				os << "\tfile_exists:\t" << (this->getRawDatau64() ? "Yes" : "No") << std::endl;
				break;
			case get_file_stats:
				os << "\tfile_len:\t" << this->getRawDatau64() << std::endl;
				break;
			default:
				os << "\tdata:\t\t" << this->getStringData() << std::endl;
				break;
		}		
	}
	else if (this->getOperation() == get_file_stats) {
		os << "\tfilename:\t" << this->getStringData() << std::endl;	
	}

	os << "}\n" << std::endl;
	
	return os;
}



// uint8_t requester_action(DirectStoragePacket *packet, uint8_t should_deliver_response) {
// 	uint8_t opcode = should_deliver_response & 0b111;
// 	if (opcode == 0) {
// 		printf("response_code: %c\n", (packet->data[0]));
// 		return 0;
// 	}
// 	switch(opcode) {
// 		case read_file:
// 			// we're reading a file and this is the first packet
// 			if (packet->sequence_bytes < sizeof(packet->data)) {
// 				#ifdef ARDUINO
//           Serial.print(F("file contents: "));  
//         #endif
// 			}
		
// 			// We're reading a file and this isn't the last packet
// 			if (packet->total_bytes - packet->sequence_bytes > 0) {
// 				should_deliver_response = ((directStorage & 0b11) << 3) | (read_file &0b111);
//         #ifdef ARDUINO
// 				Serial.print(packet->data);
//         #endif
// 			}
// 			// Last packet has arrived, transfer over
// 			else {
//         #ifdef ARDUINO
// 				Serial.println(packet->data);
//         #endif
// 				should_deliver_response = 0;
// 			}
// 			break;
	
// 		case get_file_stats:
// 			printf("check_radio newFileSize: %lu\n", (uint32_t)packet->data[0] |
//         (uint32_t)packet->data[1] << 8 |
//         (uint32_t)packet->data[2] << 16 |
//         (uint32_t)packet->data[3] << 24 );
// 			should_deliver_response = 0;
// 			break;
// 		default:
// 			should_deliver_response = 0;
// 			printf("response_code: %c\n", packet->data[0]);
// 			break;
// 	}
// 	return should_deliver_response;
// }



uint8_t DSPacket::responder_action(NRFPacket ***response_packet_queue, uint64_t *packet_count) const {
    if (getOperation() == file_exists) {
      *packet_count = 2;
      DSPacket **response_queue = (DSPacket **) malloc(sizeof(DSPacket *) * (*packet_count));
      if (!response_queue) {
      	*packet_count = -1;
      	*response_packet_queue = NULL;
      	return 0;

      }
      for (uint8_t i = 0; i < *packet_count; i++) {
      	response_queue[i] = new DSPacket();
      	if (!response_queue[i]) {
      		*packet_count = -1;
      		*response_packet_queue = NULL;
      		while (i--)
      			delete response_queue[i];

      		return 0;
      	}
      }
      *response_packet_queue = (NRFPacket **) response_queue;

      response_queue[0]->setIsResponse(1);
      response_queue[0]->setOperation(getOperation());
      response_queue[0]->setRawDatau64(1);

      response_queue[1]->setIsResponse(2);
      response_queue[1]->setOperation(getOperation());
      response_queue[1]->setRawDatau64(5);

      //#ifdef HAS_SD
      // bool file_exists = SD.exists(packet->filename);
      // Serial.print(F("file_exists: "));
      // Serial.println(file_exists);
      // packet->data[0] = file_exists ? 'y' : 'n';
      // packet->file_operation = response;
      // send_packet((GeneralPacket *)packet);
      // #endif
    }
    return 0;
    // else if (packet->file_operation == create_file) {
    //   #ifdef HAS_SD
    //   bool file_exists = SD.exists(packet->filename);
    //   printf("file_exists: %d\n", file_exists);
    //   delay_us();
    //   myFile = SD.open(packet->filename, FILE_WRITE);
    //   myFile.println("new file");
    //   myFile.close();
    //   packet->data[0] = file_exists ? 'n' : 'y';
    //   packet->data[1] = '\0';
    //   packet->file_operation = response;
    //   send_packet((GeneralPacket *)packet);
    //   *should_deliver_response = 0;
    //   #endif
    // }

    // else if (packet->file_operation == delete_file) {
    //   #ifdef HAS_SD
    //   Serial.println(F("running delete operation"));
    //   SD.remove(packet->filename);
    //   delay_us();
    //   packet->data[0] = 'y';
    //   packet->data[1] = '\0';
    //   packet->file_operation = response;
    //   send_packet((GeneralPacket *)packet);
    //   *should_deliver_response = 0;
    //   #endif
    // }
  
    //  else if (packet->file_operation == get_file_stats) {
    //   #ifdef HAS_SD
    //   Serial.println(F("get_file_stats"));
    //   delay_us();
    //   myFile = SD.open(packet->filename);
    //   unsigned long file_size = myFile.size() - 2;
    //   Serial.print(F("file_size: "));
    //   Serial.println(file_size);
    //   while (myFile.available()) {
    //       Serial.write(myFile.read());
    //   }
    //   Serial.println(F("done printing"));
    //   myFile.close();
    //   Serial.print(F("responding file_size: "));
    //   Serial.println(file_size);
    //   packet->data[0] = file_size & 0xFF;
    //   packet->data[1] = file_size >> 8 & 0xFF;
    //   packet->data[2] = file_size >> 16 & 0xFF;
    //   packet->data[3] = file_size >> 24 & 0xFF;
    //   packet->file_operation = response;
    //   send_packet((GeneralPacket *)packet);
    //   *should_deliver_response = 0;
    //   #endif
    // }
    
    // else if (packet->file_operation == read_file) {
    //   #ifdef HAS_SD
    //   Serial.println(F("running read_file"));
    //   packet->file_operation = response;
    //   for(uint8_t i = 0; i < sizeof(packet->data); i++) { packet->data[i] = 0;}
      
    //   myFile = SD.open(packet->filename);
      
    //   packet->total_bytes = myFile.size() - 2;
    //   for(uint8_t i = 0; i < sizeof(packet->filename); i++) { packet->filename[i] = 0;}
    //   uint32_t file_cursor = 0;
    //   uint32_t packet_num = 0;
    //   for (uint16_t i = 0; i < packet->total_bytes; i++) {
    //     packet->data[file_cursor] = myFile.read();
    //     if (file_cursor == sizeof(packet->data) - 2) {
    //         packet_num++;
    //         packet->sequence_bytes = packet_num * (sizeof(packet->data) - 1);
    //         packet->data[file_cursor + 1] = '\0';
    //         Serial.print(F("Packet data: "));
    //         Serial.println(packet->data);    
    //         Serial.print(F("sequence_bytes: "));  
    //         Serial.println(packet->sequence_bytes);    
    //         Serial.print(F("total_bytes: "));
    //         Serial.println(packet->total_bytes);
    //         delay_us();
    //         print_ds_packet(packet);
    //         send_packet((GeneralPacket *)packet);
            
    //         should_deliver_response = false;
           
    //         if (!myFile.available()) {
    //           myFile.close();
    //           return;    
    //         }
    //         file_cursor = -1;
    //     }
    //     file_cursor++;
    //   }

    //   delay_us();
    //   packet->sequence_bytes =  (packet_num * (sizeof(packet->data) - 1)) + file_cursor;
    //   packet->data[file_cursor] = '\0';
    //   Serial.print(F("Packet data: "));
    //   Serial.println(packet->data);
    //   Serial.print(F("sequence_bytes: "));  
    //   Serial.println(packet->sequence_bytes);    
    //   Serial.print(F("total_bytes: "));
    //   Serial.println(packet->total_bytes);
    //   print_ds_packet(packet);
    //   send_packet((GeneralPacket *)packet);
      
    //   myFile.close();
    //   *should_deliver_response = 0;
    //   #endif
    // }

    // Serial.println(F("sent response_packet"));

}




#endif