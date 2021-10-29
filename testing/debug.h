#ifndef DEBUG_H
#define DEBUG_H

#include "../protocols/direct_storage_packet.h"

void print_packet(GeneralPacket *packet);

void print_ds_packet(DirectStoragePacket *packet);

#endif