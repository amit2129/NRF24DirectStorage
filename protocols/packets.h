typedef enum {infiniband, directStorage} protocol_type;
static const char *protocol_type_str[] =
        { "Infiniband", "Direct Storage"};


typedef struct _general_packet { 
	uint8_t protocol:2;
	uint32_t reserved:30;
	char data[30];
} GeneralPacket;

