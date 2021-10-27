
uint8_t parseInputAction(PacketAction *action) {
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
