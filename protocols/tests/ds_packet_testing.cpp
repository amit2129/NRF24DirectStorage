#include <iostream>

#include "../ds_packet.h"

int main() {
	const DSPacket packet = DSPacket(get_file_stats, 1, 9);
	DSPacket packet2 = DSPacket(read_file, 1, "the contents");
	std::cout << packet;
	std::cout << packet.getRawString();
	std::cout << packet2;
	std::cout << packet2.getRawString();
	char packet_data[32];
	packet.getRawData(packet_data);
	const NRFPacket *nrf_packet = &packet;
	std::cout << DSPacket(get_file_stats, 1, (uint64_t )4790013 << 10);
	std::cout << DSPacket(get_file_stats, 0, "hello");

	NRFPacket* (*packetQueue) = NULL;
	uint64_t packet_num = 0;
	DSPacket file_exists_request = DSPacket(file_exists, 0);
	file_exists_request.responder_action(&packetQueue, &packet_num);

	if (packetQueue) {
		std::cout << std::endl << "PacketNum: " << packet_num << std::endl;
		std::cout << std::endl << "Packets: " << std::endl;
		for (int i = 0; i < packet_num; i++) {
			std::cout << *packetQueue[i];
			delete packetQueue[i];
		}
		free(packetQueue);
	}

	std::cout << "sizeof Packets: " << std::endl;
	std::cout << "NRFPacket: " << sizeof(NRFPacket) << std::endl;
	std::cout << "DSPacket: " << sizeof(DSPacket) << std::endl;
	std::cout << std::endl;

}
