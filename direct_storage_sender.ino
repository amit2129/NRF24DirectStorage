/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 
 Created Dec 2014 - TMRh20
 */

/**
 * Example of using interrupts
 *
 * This is a very simple example of using two devices to communicate using interrupts.
 * With multiple devices, each device would need to have a separate reading pipe
 */

#include <SPI.h>
#define HAS_SD  
#define DEBUG

#ifdef HAS_SD
  #include <SD.h>
#endif
#include "RF24.h"
#include <printf.h>

// Hardware configuration
// Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(9,10);
                                        
// Use the same address for both devices
uint8_t address[] = { "radio" };

typedef enum { role_sender = 1, role_receiver} role_e; // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "sender", "receiver"};  // The debug-friendly names of those roles
role_e role = role_receiver;

volatile uint32_t round_trip_timer = 0;

typedef enum {infiniband, directStorage} protocol_type;

typedef struct _general_packet { 
  uint8_t protocol:2;
  uint32_t reserved:30;
  char data[30];
} GeneralPacket;


typedef enum {file_exists, create_file, delete_file, append_data, get_file_stats, read_file, list_files, response} file_op;

typedef struct _direct_storage_packet { 
  uint8_t protocol:2;  
  uint8_t file_operation:3;
  uint8_t filename_len:3;
  uint16_t total_bytes;
  uint16_t sequence_bytes;
  char filename[8];
  char data[19];
} DirectStoragePacket;

typedef struct _ib_packet { 
  uint8_t protocol:2;
  uint8_t src_qp:2;
  uint8_t dst_qp:2;
  uint8_t seq_num:2;
  uint8_t ack_num:2;
  uint8_t data_len:5;
  uint8_t reserved:1;
  char data[30];
} InfinibandPacket;


typedef struct _parsed_action {
  uint8_t protocol:2;
  uint8_t action:3;
  char input_str[20];
} ParsedAction;



/********************** Setup *********************/
//volatile DirectStoragePacket ds_packet;

void check_radio(void);
void write_data(InfinibandPacket *packet, char *data, uint8_t data_len);
void print_ds_packet(DirectStoragePacket *packet);
void print_infiniband_packet(InfinibandPacket *packet);
void print_packet(GeneralPacket *packet);
void send_packet(GeneralPacket *packet);
uint8_t parseInputAction(ParsedAction *action);


#ifdef HAS_SD
File myFile;
#endif
void setup(){

  Serial.begin(115200);
  printf_begin();
  
  // Setup and configure rf radio
  radio.begin();

  // Use dynamic payloads to improve response time
  radio.enableDynamicPayloads();
  radio.openWritingPipe(address);             // communicate back and forth.  One listens on it, the other talks to it.
  radio.openReadingPipe(1,address); 
  radio.startListening();
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging

  attachInterrupt(0, check_radio, LOW);             // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver

//  ds_packet.total_bytes = 0;
//  ds_packet.sequence_bytes = 0;
//  ds_packet.protocol = directStorage;
//  ds_packet.file_operation = file_exists;
//  char *filename = "test.t";
//  ds_packet.filename_len = strlen(filename) - 1;
//  printf("filename_len: %d\n", strlen(filename));
//  strcpy(ds_packet.filename, filename);
//  printf("sizeof packet: %d\n", sizeof(DirectStoragePacket));

  #ifdef HAS_SD
  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  SD.remove("test.t");
  myFile = SD.open("test.t", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to test.t...");
    myFile.println("testing 1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
    myFile.close();
    myFile = SD.open("test.t");
    Serial.print("file_size: ");
    Serial.println(myFile.size());
    Serial.println("file_contents:");
    int counter = 0;
    while (myFile.available()) {
//      counter++;
      Serial.write(myFile.read());
//      Serial.print(myFile.read());
    }
    Serial.print("byte counter: ");
    Serial.println(counter);
    myFile.close();
    Serial.println();
  }
  #else
  Serial.println("No SD Card");
  #endif
}


uint8_t parseInputAction(ParsedAction *action) {
  uint32_t initial_micros = micros();
  if (!Serial.available()) {
    return -1;
  }

  else {
    printf("parsing input\n\n");
    while(!Serial.available()) { }
    action->protocol = (uint8_t )Serial.read() - '0';
    while(!Serial.available()) { }
    action->action = (uint8_t)Serial.read() - '0';
    uint8_t name_cursor = 0;
    while(true) {
      while(!Serial.available()) { }
      char read_char = Serial.read();
      if (read_char == '\n') {
        action->input_str[name_cursor] = '\0';
        break;
      }
      action->input_str[name_cursor++] = read_char;
    }
  }
  uint32_t diff_micros = micros() - initial_micros;
  printf("protocol:\t%d\naction:\t\t%d\ninput_str:\t%s\n\nprocessing_time: %lu microseconds\n\n", action->protocol, action->action, action->input_str, diff_micros);
  return 0;
}

//uint8_t parse_input() {
//  if (!Serial.available()) {
//    return 0;
//  }
//  if(Serial.available()){
//    switch(toupper(Serial.read())){
//      case 'T': 
//        delay(1000);
//        char file_name[8];
//        uint8_t name_cursor = 0;
//        while(Serial.available()) {
//          char read_char = Serial.read();
//          printf("read_char: %c", read_char);
//          file_name[name_cursor++] = read_char;
//        }
//        printf("\n");
//        file_name[name_cursor] = '\0';
//        ds_packet.filename_len = strlen(file_name) - 1;
//        strcpy(ds_packet.filename, file_name);
//        while(micros() - round_trip_timer < 45000){
//        }
//        send_packet((GeneralPacket *)&ds_packet);
//        break;
//    }
//  }
//}


uint32_t send_time;

void performDSAction(file_op file_operation, char *input_str) {
    DirectStoragePacket ds_packet;
    ds_packet.total_bytes = 0;
    ds_packet.sequence_bytes = 0;
    ds_packet.protocol = directStorage;
    ds_packet.file_operation = file_operation;
    ds_packet.filename_len = strlen(input_str) - 1;
    strcpy(ds_packet.filename, input_str);
    for (uint8_t i = 0; i < sizeof(ds_packet.data); i++) { ds_packet.data[i] = 0;}
    send_time = micros();
    Serial.println("Sending:");
    print_ds_packet(&ds_packet);
    send_packet((GeneralPacket *)&ds_packet);
}
/********************** Main Loop *********************/
void loop() {
    ParsedAction parsedAction;
    if (parseInputAction(&parsedAction) == 0) {
      switch(parsedAction.protocol){
        case directStorage:
          delay_us();
          performDSAction(parsedAction.action, parsedAction.input_str);
          // printf("\ndetected directStorageAction: %d with data: %s\n", parsedAction.action, parsedAction.input_str);
          break;
        case 2:
          while(radio.available()) {
            DirectStoragePacket tempPacket;
            radio.read(&tempPacket, sizeof(tempPacket));
          }
      }
    }
// uint8_t ret = parse_input();
}   

volatile uint8_t should_deliver_response = 0;

void prepare_deliver_response(GeneralPacket *packet) {
  if (packet->protocol == directStorage) {
    if (((DirectStoragePacket *)packet)->file_operation == response) {
      return;
    }
    should_deliver_response = ((packet->protocol & 0b11) << 3) | (((DirectStoragePacket *)packet)->file_operation &0b111);
   //printf("should_deliver_response:");
   //bin(should_deliver_response);
   printf("\n");
  } 
}

void send_packet(GeneralPacket *packet) {
  prepare_deliver_response(packet);
//  radio.stopListening();                
  round_trip_timer = micros();
  radio.startWrite(packet, sizeof(*packet),0);
}
/********************** Interrupt *********************/

void check_radio(void)
{
  
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

 
  // If data is available, handle it accordingly
  if ( rx ){
    //Serial.println(F("rx event occurred"));
    
    if(radio.getDynamicPayloadSize() < 1){
      // Corrupt payload has been flushed
      return; 
    }
    GeneralPacket packet;
    radio.read(&packet,sizeof(packet));
    
    if (packet.protocol == directStorage) {
   //   printf("protocol is directStorage\n");
      if (!should_deliver_response) {
        print_ds_packet((DirectStoragePacket *)&packet);
      }
      parse_ds_packet((DirectStoragePacket *)&packet);
    }
    else {
      printf("received other packet of protocol: %d\n", packet.protocol);
      print_infiniband_packet((InfinibandPacket *)&packet);
    }
    if (should_deliver_response) {
        //Serial.println("should_deliver_response");
   //   printf("receiver resp and should_deliver_response is: \n");
  //    bin(should_deliver_response);
   //   printf("\n");

      if (packet.protocol == directStorage) {
        uint8_t opcode = should_deliver_response & 0b111;
        DirectStoragePacket *packet_ds = (DirectStoragePacket *)&packet;
        if (opcode == 0) {
           printf("response_code: %c\n", ((DirectStoragePacket *)&packet)->data[0]);
           return;
        }
        switch(opcode) {
          case read_file:
            if (packet_ds->sequence_bytes < sizeof(packet_ds->data)) {
               Serial.print(F("file contents: "));  
            }
            
            if (packet_ds->total_bytes - packet_ds->sequence_bytes > 0) {
              should_deliver_response = ((directStorage & 0b11) << 3) | (read_file &0b111);
              Serial.print(packet_ds->data);
            }
            else {
               Serial.println(packet_ds->data);
               should_deliver_response = 0;
            }
            //Serial.print(F("packet_ds->total_bytes: "));
            //Serial.println(packet_ds->total_bytes);

            //Serial.print(F("packet_ds->sequence_bytes: "));
            //Serial.println(packet_ds->sequence_bytes);

            
            
            break;
          case get_file_stats:
            unsigned long newFileSize = (uint32_t)packet_ds->data[0] |
                                        (uint32_t)packet_ds->data[1] << 8 |
                                        (uint32_t)packet_ds->data[2] << 16 |
                                        (uint32_t)packet_ds->data[3] << 24; 
            printf("check_radio newFileSize: %lu\n", newFileSize);
            should_deliver_response = 0;
            break;
          default:
            should_deliver_response = 0;
            printf("response_code: %c\n", ((DirectStoragePacket *)&packet)->data[0]);
            break;
        }
      }
    }
  }

  // Start listening if transmission is complete
  else if( tx || fail ){
     Serial.print(F("tx: \n"));
     radio.startListening(); 
     Serial.println(tx ? F(":OK") : F(":Fail"));
     if (!tx) {
      printf("should_deliver_response: %d\n", should_deliver_response);
      should_deliver_response = 0;
     }
  }  
}

void bin(uint8_t n)
{
    uint8_t i;
    for (i = 1 << 7; i > 0; i = i / 2)
        (n & i) ? printf("1") : printf("0");
}




void write_data(InfinibandPacket *packet, char *data, uint8_t data_len) {
  memcpy((uint8_t *)packet->data, (uint8_t *)data, data_len);
  packet->data[data_len] = '\0';
  packet->data_len = data_len;
}


void parse_ds_packet(DirectStoragePacket *packet) {
  //  printf("operation: %d\n", packet->file_operation);
    if (packet->file_operation == response) {
      
      return;
    }

    if (packet->file_operation == file_exists) {
      #ifdef HAS_SD
      bool file_exists = SD.exists(packet->filename);
      Serial.print(F("file_exists: "));
      Serial.println(file_exists);
      delay_us();

      packet->data[0] = file_exists ? 'y' : 'n';
      packet->file_operation = response;    
      send_packet((GeneralPacket *)packet);
      should_deliver_response = false;
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
      should_deliver_response = false;
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
      should_deliver_response = false;
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
      should_deliver_response = false;
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
      should_deliver_response = false;
      #endif
    }

    Serial.println(F("sent response_packet"));

}


void delay_us() {
        radio.stopListening();
      for(uint32_t i=0; i<130;i++){
         __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
      }
}


void print_packet(GeneralPacket *packet) {
  #ifdef DEBUG
  Serial.println(F("Packet: {"));
  Serial.print(F("\tprotocol:\t\t"));
  Serial.println(packet->protocol);
  Serial.println("}");
  #endif
}

void print_infiniband_packet(InfinibandPacket *packet) {
  #ifdef DEBUG
  Serial.println(F("Packet: {"));
  Serial.print(F("\tprotocol:\t"));
  Serial.println(packet->src_qp);
  Serial.print(F("\tdst_qp:\t\t"));
  Serial.println(packet->dst_qp);
  Serial.print(F("\tseq_num:\t"));
  Serial.println(packet->seq_num);
  Serial.print(F("\tack_num:\t"));
  Serial.println(packet->ack_num);
  Serial.print(F("\tdata_len:\t"));
  Serial.println(packet->data_len);
  Serial.print(F("\tdata:\t\t"));
  Serial.println(packet->data);
  Serial.println("}");
  #endif
}

void print_ds_packet(DirectStoragePacket *packet) {
  #ifdef DEBUG
  Serial.println(F("Packet: {"));
  Serial.print(F("\tprotocol:\t"));
  Serial.println(packet->protocol);
  Serial.print(F("\tfile_op:\t"));
  Serial.println(packet->file_operation);
  Serial.print(F("\tfilename_len:\t"));
  Serial.println(packet->filename_len);
  Serial.print(F("\ttot_bytes:\t"));
  Serial.println(packet->total_bytes);
  Serial.print(F("\tsequence_bytes:\t"));
  Serial.println(packet->sequence_bytes);
  Serial.print(F("\tfilename:\t"));
  Serial.println(packet->filename);
  Serial.print(F("\tdata:\t"));
  Serial.println(packet->data);
  Serial.println("}");
  #endif
}
