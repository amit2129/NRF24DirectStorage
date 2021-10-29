#ifndef DIRECT_STORAGE_PACKET
#define DIRECT_STORAGE_PACKET
#include <stdint.h>
#include <sstream>
#include <string.h>
#include "../utils.h"
#include "nrf_packet.h"


#define PACKET_SIZE 32
#define PACKET_BASE_OFFSET 1

enum file_op {file_exists, create_file, delete_file, append_data, get_file_stats, read_file, list_files, response};
static const char *file_op_str[] =
        { "File Exists", "Create File", "Delete File", "Append Data", "File Size", "Read File", "List Files", "Response"};


void perform_ds_action(PacketAction *action);

//void parse_ds_request(DirectStoragePacket *packet, uint8_t *should_deliver_response);


class DSPacket: public NRFPacket {
	public:
		// DSPacket() {}
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

		uint8_t responder_action(NRFPacket ***response_packet_queue, uint64_t *packet_count) const;
		uint8_t setRawData(char packet_data[], uint8_t data_len);
		std::string getProtocolName(void) const { return "DSPacket";}

		virtual std::ostream & output( std::ostream & ) const;


	private:
		uint8_t requester_action(void) { return 0; }
};



//inline std::ostream & operator<<(std::ostream & os, DSPacket const & packet);
// std::ostream & operator <<( std::ostream &os, const DSPacket &packet )
// {
    // return packet.out(os);
// }



#endif