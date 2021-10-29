#include "direct_storage_packet.h"
#include "../testing/debug.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern send_packet(GeneralPacket *packet);

void perform_ds_action(PacketAction *action) {
	DirectStoragePacket ds_packet;
	ds_packet.total_bytes = 0;
	ds_packet.sequence_bytes = 0;
	ds_packet.protocol = directStorage;
	ds_packet.file_operation = (file_op) action->action;
	for (uint8_t i = 0; i < sizeof(ds_packet.data); i++) { ds_packet.data[i] = 0;}
  #ifdef ARDUINO
	Serial.println("Sending:");
  #endif
	print_ds_packet(&ds_packet);
	send_packet((GeneralPacket *)&ds_packet);
}


bool handle_ds_response(DirectStoragePacket *packet, uint8_t should_deliver_response) {
	uint8_t opcode = should_deliver_response & 0b111;
	if (opcode == 0) {
		printf("response_code: %c\n", (packet->data[0]));
		return 0;
	}
	switch(opcode) {
		case read_file:
			// we're reading a file and this is the first packet
			if (packet->sequence_bytes < sizeof(packet->data)) {
				#ifdef ARDUINO
          Serial.print(F("file contents: "));  
        #endif
			}
		
			// We're reading a file and this isn't the last packet
			if (packet->total_bytes - packet->sequence_bytes > 0) {
				should_deliver_response = ((directStorage & 0b11) << 3) | (read_file &0b111);
        #ifdef ARDUINO
				Serial.print(packet->data);
        #endif
			}
			// Last packet has arrived, transfer over
			else {
        #ifdef ARDUINO
				Serial.println(packet->data);
        #endif
				should_deliver_response = 0;
			}
			break;
	
		case get_file_stats:
			printf("check_radio newFileSize: %lu\n", (uint32_t)packet->data[0] |
        (uint32_t)packet->data[1] << 8 |
        (uint32_t)packet->data[2] << 16 |
        (uint32_t)packet->data[3] << 24 );
			should_deliver_response = 0;
			break;
		default:
			should_deliver_response = 0;
			printf("response_code: %c\n", packet->data[0]);
			break;
	}
	return should_deliver_response;
}


void parse_ds_request(DirectStoragePacket *packet, uint8_t *should_deliver_response) {
    if (packet->file_operation == file_exists) {
      #ifdef HAS_SD
      bool file_exists = SD.exists(packet->filename);
      Serial.print(F("file_exists: "));
      Serial.println(file_exists);
      delay_us();

      packet->data[0] = file_exists ? 'y' : 'n';
      packet->file_operation = response;    
      send_packet((GeneralPacket *)packet);
      *should_deliver_response = 0;
      #endif
    }
    else if (packet->file_operation == create_file) {
      #ifdef HAS_SD
      bool file_exists = SD.exists(packet->filename);
      printf("file_exists: %d\n", file_exists);
      delay_us();
      myFile = SD.open(packet->filename, FILE_WRITE);
      myFile.println("new file");
      myFile.close();
      packet->data[0] = file_exists ? 'n' : 'y';
      packet->data[1] = '\0';
      packet->file_operation = response;
      send_packet((GeneralPacket *)packet);
      *should_deliver_response = 0;
      #endif
    }

    else if (packet->file_operation == delete_file) {
      #ifdef HAS_SD
      Serial.println(F("running delete operation"));
      SD.remove(packet->filename);
      delay_us();
      packet->data[0] = 'y';
      packet->data[1] = '\0';
      packet->file_operation = response;
      send_packet((GeneralPacket *)packet);
      *should_deliver_response = 0;
      #endif
    }
  
     else if (packet->file_operation == get_file_stats) {
      #ifdef HAS_SD
      Serial.println(F("get_file_stats"));
      delay_us();
      myFile = SD.open(packet->filename);
      unsigned long file_size = myFile.size() - 2;
      Serial.print(F("file_size: "));
      Serial.println(file_size);
      while (myFile.available()) {
          Serial.write(myFile.read());
      }
      Serial.println(F("done printing"));
      myFile.close();
      Serial.print(F("responding file_size: "));
      Serial.println(file_size);
      packet->data[0] = file_size & 0xFF;
      packet->data[1] = file_size >> 8 & 0xFF;
      packet->data[2] = file_size >> 16 & 0xFF;
      packet->data[3] = file_size >> 24 & 0xFF;
      packet->file_operation = response;
      send_packet((GeneralPacket *)packet);
      *should_deliver_response = 0;
      #endif
    }
    
    else if (packet->file_operation == read_file) {
      #ifdef HAS_SD
      Serial.println(F("running read_file"));
      packet->file_operation = response;
      for(uint8_t i = 0; i < sizeof(packet->data); i++) { packet->data[i] = 0;}
      
      myFile = SD.open(packet->filename);
      
      packet->total_bytes = myFile.size() - 2;
      for(uint8_t i = 0; i < sizeof(packet->filename); i++) { packet->filename[i] = 0;}
      uint32_t file_cursor = 0;
      uint32_t packet_num = 0;
      for (uint16_t i = 0; i < packet->total_bytes; i++) {
        packet->data[file_cursor] = myFile.read();
        if (file_cursor == sizeof(packet->data) - 2) {
            packet_num++;
            packet->sequence_bytes = packet_num * (sizeof(packet->data) - 1);
            packet->data[file_cursor + 1] = '\0';
            Serial.print(F("Packet data: "));
            Serial.println(packet->data);    
            Serial.print(F("sequence_bytes: "));  
            Serial.println(packet->sequence_bytes);    
            Serial.print(F("total_bytes: "));
            Serial.println(packet->total_bytes);
            delay_us();
            print_ds_packet(packet);
            send_packet((GeneralPacket *)packet);
            
            should_deliver_response = false;
           
            if (!myFile.available()) {
              myFile.close();
              return;    
            }
            file_cursor = -1;
        }
        file_cursor++;
      }

      delay_us();
      packet->sequence_bytes =  (packet_num * (sizeof(packet->data) - 1)) + file_cursor;
      packet->data[file_cursor] = '\0';
      Serial.print(F("Packet data: "));
      Serial.println(packet->data);
      Serial.print(F("sequence_bytes: "));  
      Serial.println(packet->sequence_bytes);    
      Serial.print(F("total_bytes: "));
      Serial.println(packet->total_bytes);
      print_ds_packet(packet);
      send_packet((GeneralPacket *)packet);
      
      myFile.close();
      *should_deliver_response = 0;
      #endif
    }

    Serial.println(F("sent response_packet"));

}

