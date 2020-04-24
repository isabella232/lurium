#pragma once

// client -> server
const int control_protocol_hello = 0; // reports ip, board sise, framerate
const int control_protocol_status = 1; // reports number of players

struct control_message_hello_v2 : data_message<control_protocol_hello> {
	uint8_t version;
	std::string protocol;
	std::string key;
	std::string mode;
	std::string region;
	std::string url;
	std::string description;
	uint16_t max_player_count;
};

template<>
inline void data_serialize(data_serializer& s, const control_message_hello_v2& m) {
	s.write(m.version);
	s.write(m.protocol);
	s.write(m.key);
	s.write(m.mode);
	s.write(m.region);
	s.write(m.url);
	s.write(m.description);
	s.write(m.max_player_count);
}

template<>
inline void data_deserialize(data_deserializer& s, control_message_hello_v2& m) {
	s.read(m.version);
	s.read(m.protocol);
	s.read(m.key);
	s.read(m.mode);
	s.read(m.region);
	s.read(m.url);
	s.read(m.description);
	s.read(m.max_player_count);
}

struct control_message_status : data_message<control_protocol_status> {
	uint16_t player_count;
};

template<>
inline void data_serialize(data_serializer& s, const control_message_status& m) {
	s.write(m.player_count);
}

template<>
inline void data_deserialize(data_deserializer& s, control_message_status& m) {
	s.read(m.player_count);
}
