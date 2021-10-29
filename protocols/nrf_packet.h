#ifndef NRF_PACKET
#define NRF_PACKET

#include <stdint.h>
#include <iostream>


class NRFPacket {
	protected:
		uint8_t data[32];

	public:
		NRFPacket() { for (int i = 0; i < 32; i++) { data[i] = 0;}}

		void setOperation(uint8_t op);
		void setProtocol(uint8_t proto);
		uint8_t getOperation(void) const;
		uint8_t getProtocol(void) const;
		
		uint8_t isResponse(void) const;
		void setIsResponse(uint8_t isResponse);

		virtual uint8_t responder_action(NRFPacket ***response_packet_queue, uint64_t *packet_count) const = 0;
		//virtual uint8_t responder_action(void) = 0;
		virtual uint8_t requester_action(void) = 0;
		virtual std::string getProtocolName(void) const = 0;
		virtual std::ostream & output( std::ostream & ) const;

	private:
		uint8_t parse_request(void);
};


inline std::ostream & operator<<(std::ostream & os, NRFPacket const & packet);
std::ostream & operator<<(std::ostream & os, NRFPacket const & packet) {
	return packet.output( os );
}



#endif