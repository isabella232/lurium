#pragma once

// read and write DataView

struct binary_union {
	union {
		uint8_t c[4];
		float f;
		uint32_t u;
		int32_t s;
		uint16_t u16[2];
		int16_t s16[2];
	};
};

struct data_serializer {
	std::string& buffer;
	std::string::iterator pos;

	data_serializer(std::string& s) : buffer(s) {
		pos = buffer.end();
	}

	void write(uint8_t c) {
		write_uint8(c);
	}

	void write_uint8(uint8_t c) {
		if (pos == buffer.end()) {
			buffer.push_back(c);
			pos = buffer.end();
		} else {
			*pos = c;
			pos++;
		}
	}

	void write(bool b) {
		write_uint8(b ? 1 : 0);
	}

	void write(uint16_t c) {
		write_uint16(c);
	}
	
	void write_uint16(uint16_t c) {
		binary_union b;
		b.u16[0] = c;
		write_uint8(b.c[1]);
		write_uint8(b.c[0]);
	}

	void write_uint32(uint32_t c) {
		binary_union b;
		b.u = c;
		write_binary(b);
	}

	void write_int32(uint32_t c) {
		binary_union b;
		b.s = c;
		write_binary(b);
	}

	void write(float c) {
		write_float(c);
	}

	void write_float(float c) {
		binary_union b;
		b.f = c;
		write_binary(b);
	}

	void write(const std::string& s) {
		write_string(s);
	}

	void write_string(const std::string& s) {
		binary_union b;
		b.u = s.length();
		write_binary(b);
		for (std::string::const_iterator i = s.begin(); i != s.end(); ++i) {
			write_uint8(*i);
		}
	}

	void write_binary(binary_union b) {
		write_uint8(b.c[3]);
		write_uint8(b.c[2]);
		write_uint8(b.c[1]);
		write_uint8(b.c[0]);
	}
};

struct data_deserializer {
	const uint8_t* buffer;
	size_t length;
	size_t pos;
	bool error;

	data_deserializer(const std::string& s) :buffer((const uint8_t*)s.c_str()), length(s.length()) {
		pos = 0;
		error = false;
	}

	data_deserializer(const uint8_t* s, size_t len):buffer(s), length(len) {
		pos = 0;
		error = false;
	}

	bool read(uint8_t& b) {
		if (pos == length) {
			error = true;
		} else {
			b = buffer[pos];
			pos++;
		}
		return !error;
	}

	bool read(bool& b) {
		uint8_t t;
		if (read(t)) {
			b = t != 0;
			return true;
		} else {
			return false;
		}
	}
	/*
	uint8_t read_uint8() {
		uint8_t c;
		if (!read(c)) {
			return 0;
		}
		return c;*/
/*		if (pos == length) {
			error = true;
			return 0;
		}

		uint8_t c = buffer[pos];
		pos++;
		return c;*/
	//}

	bool read(uint16_t& result) {
		binary_union b;
		read(b.c[1]);
		read(b.c[0]);
		result = b.u16[0];
		return !error;
	}

	bool read(uint32_t& result) {
		binary_union b;
		read(b.c[3]);
		read(b.c[2]);
		read(b.c[1]);
		read(b.c[0]);
		result = b.u;
		return !error;
	}

	bool read(int32_t& result) {
		binary_union b;
		read(b.c[3]);
		read(b.c[2]);
		read(b.c[1]);
		read(b.c[0]);
		result = b.s;
		return !error;
	}

	bool read(float& result) {
		binary_union b;
		read(b.c[3]);
		read(b.c[2]);
		read(b.c[1]);
		read(b.c[0]);
		result = b.f;
		return !error;
	}

	bool read(std::string& result) {
		uint32_t bytes;// = read_uint32();
		read(bytes);
		if (pos + bytes > length) {
			pos = length;
			error = true;
			return false;// return std::string();
		}

		//std::string result;
		uint8_t c;
		for (uint32_t i = 0; i < bytes; i++) {
			read(c);
			result += (std::string::value_type)c;
		}
		return !error;
		//return result;
	}
};

template<uint8_t OP>
struct data_message {
	static const uint8_t op() { return OP; }
};

template<typename T>
inline void data_serialize(data_serializer& s, const T& m) {
	s.write(m);
}

template<typename T>
inline void data_deserialize(data_deserializer& s, T& m) {
	s.read(m);
}

template<typename T>
inline void data_serialize(data_serializer& s, const std::vector<T>& m) {
	s.write((uint16_t)m.size());
	for (auto& i : m) {
		data_serialize(s, i);
	}
}

template<typename T>
inline void data_deserialize(data_deserializer& s, std::vector<T>& m) {	
	uint16_t length;
	s.read(length);
	for (uint32_t i = 0; i < length; i++) {
		T d;
		data_deserialize(s, d);
		m.push_back(d);
	}
}
