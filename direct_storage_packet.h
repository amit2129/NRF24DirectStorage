
typedef enum {file_exists, create_file, delete_file, append_data, get_file_stats, read_file, list_files, response} file_op;


typedef struct _direct_storage_packet { 
	ptotocol_type protocol:4;  
	file_op file_operation:4;
	uint16_t total_bytes;
	uint16_t sequence_bytes;
	char data[27];
} DirectStoragePacket;
