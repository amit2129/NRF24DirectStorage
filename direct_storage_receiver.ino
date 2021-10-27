#include <SPI.h>
#define HAS_SD  
#define DEBUG

#ifdef HAS_SD
#include <SD.h>
#endif
#include "RF24.h"
#include <printf.h>


RF24 radio(9,10);

// Use the same address for both devices
uint8_t address[] = { "radio" };



typedef struct _packet_action {
	uint8_t protocol:2;
	uint8_t action:3;
	char input_str[20];
} PacketAction;



/********************** Setup *********************/
//volatile DirectStoragePacket ds_packet;

void check_radio(void);
void write_data(InfinibandPacket *packet, char *data, uint8_t data_len);
void print_ds_packet(DirectStoragePacket *packet);
void print_infiniband_packet(InfinibandPacket *packet);
void print_packet(GeneralPacket *packet);
void send_packet(GeneralPacket *packet);
uint8_t parseInputAction(PacketAction *action);


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
	radio.openWritingPipe(address);
	radio.openReadingPipe(1,address); 
	radio.startListening();
	radio.printDetails()

	attachInterrupt(0, check_radio, LOW);

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



uint32_t send_time;



void loop() {
	PacketAction parsedAction;
	if (parseInputAction(&parsedAction) == 0) {
		switch(parsedAction.protocol){
			case directStorage:
				delay_us();
				perform_ds_action(&parsedAction);
				break;
		}
	}
}   

volatile uint8_t should_deliver_response = 0;

void prepare_deliver_response(GeneralPacket *packet) {
	if (packet->protocol == directStorage) {
		if (((DirectStoragePacket *)packet)->file_operation == response) {
			return;
		}
		should_deliver_response = ((packet->protocol & 0b11) << 3) | (((DirectStoragePacket *)packet)->file_operation &0b111);
	} 
}

void send_packet(GeneralPacket *packet) {
	prepare_deliver_response(packet);
	radio.startWrite(packet, sizeof(*packet),0);
}



void check_radio(void)
{

	bool tx,fail,rx;
	radio.whatHappened(tx,fail,rx);


	if ( rx ){

		if(radio.getDynamicPayloadSize() < 1){
			// corrupt payload is ignored
			return; 
		}
		GeneralPacket packet;
		radio.read(&packet,sizeof(packet));

		if (packet.protocol == directStorage) {
			DirectStoragePacket *packet_ds = (DirectStoragePacket *)&packet;
			if (!should_deliver_response) {
				print_ds_packet(ds_packet);
			}
			if (packet_ds->file_operation != response)
				parse_ds_packet(ds_packet, &should_deliver_response);
		}
		else{
			if (should_deliver_response) {  	
				should_deliver_response = handle_ds_response(packet_ds);
			}
		}
	}
	else {
		printf("received other packet of protocol: %d\n", packet.protocol);
	}

	if (should_deliver_response) {
		if (packet.protocol == directStorage) {
			DirectStoragePacket *packet_ds = (DirectStoragePacket *)&packet;
			should_deliver_response = handle_ds_response(packet_ds);
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





