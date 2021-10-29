#ifndef UTILS
#define UTILS

typedef struct _packet_action {
	uint8_t protocol:4;
	uint8_t action:4;
	char input_str[20];
} PacketAction;


#endif 