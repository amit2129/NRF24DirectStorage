
#include "debug.h"


void print_packet(GeneralPacket *packet) {
  #ifdef DEBUG
  Serial.println(F("Packet: {"));
  Serial.print(F("\tprotocol:\t\t"));
  Serial.println(packet->protocol);
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
