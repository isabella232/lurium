#pragma once

// cross platform websocket interface

// emscripten implements the c api in websocket.js, same api using websocketpp implemented in websocket.cpp

#if defined(__EMSCRIPTEN__)
typedef int websocket_handle_t;
#else
struct websocket_impl;
typedef websocket_impl* websocket_handle_t;
#endif

extern "C" websocket_handle_t websocket_create(const char* url, void* userdata, void(*onopen)(void*), void(*onclose)(void*), void(*onerror)(void*), void(*onmessage)(void*, const char*, int));
extern "C" void websocket_send(websocket_handle_t handle, const char* buffer, int length);
extern "C" void websocket_close(websocket_handle_t handle);

// websocket_client is a helper for serializing access to websocket events

enum message_type {
	message_type_connect,
	message_type_disconnect,
	message_type_message
};

struct message_buffer {
	message_type type;
	std::shared_ptr<std::vector<char>> buffer;

	message_buffer(const char* _buffer, int _length) {
		type = message_type::message_type_message;
		buffer = std::make_shared<std::vector<char>>(_buffer, _buffer + _length);
	}

	message_buffer(message_type _type) {
		type = _type;
	}
};

#include <mutex>

struct websocket_client {
	websocket_handle_t handle;
	bool is_connected;
	std::queue<message_buffer> queue;
	std::mutex mutex;

	websocket_client() : handle(NULL), is_connected(false) {}

	virtual ~websocket_client() {
		close();
	}

	static void onopen(void* userdata) {
		websocket_client* self = (websocket_client*)userdata;
		std::lock_guard<std::mutex> lock(self->mutex);
		self->is_connected = true;
		self->queue.push(message_buffer(message_type::message_type_connect));
	}

	static void onclose(void* userdata) {
		websocket_client* self = (websocket_client*)userdata;
		std::lock_guard<std::mutex> lock(self->mutex);
		self->is_connected = false;
		self->queue.push(message_buffer(message_type::message_type_disconnect));
	}

	static void onerror(void* userdata) {
		websocket_client* self = (websocket_client*)userdata;
		std::lock_guard<std::mutex> lock(self->mutex);
		self->is_connected = false;
		self->queue.push(message_buffer(message_type::message_type_disconnect));
	}

	static void onmessage(void* userdata, const char* buffer, int length) {
		websocket_client* self = (websocket_client*)userdata;
		std::lock_guard<std::mutex> lock(self->mutex);
		self->queue.push(message_buffer(buffer, length));
	}

	void open(const std::string& url) {
		handle = websocket_create(url.c_str(), this, onopen, onclose, onerror, onmessage);
	}

	void close() {
		if (handle != NULL) {
			websocket_close(handle);
			handle = NULL;
		}
	}

	template <typename T>
	void send(T& m) {
		std::string buffer;
		data_serializer s(buffer);

		s.write_uint8(T::op());
		data_serialize(s, m);

		websocket_send(handle, buffer.c_str(), buffer.size());
	}

	template <typename MSGCB, typename OPENCB, typename CLOSECB>
	void poll(MSGCB && message_callback, OPENCB && open_callback, CLOSECB && close_callback) {
		std::lock_guard<std::mutex> lock(mutex);
		while (!queue.empty()) {
			const message_buffer& msg = queue.front();
			if (msg.type == message_type::message_type_message) {
				const std::vector<char>& buffer = *queue.front().buffer;
				data_deserializer des((const uint8_t*)&buffer.front(), buffer.size());
				message_callback(des);
			}
			else if (msg.type == message_type::message_type_connect) {
				open_callback();
			}
			else if (msg.type == message_type::message_type_disconnect) {
				close_callback();
			}
			else {
				assert(false);
			}
			queue.pop();
		}
	}

	template <typename T, typename CB>
	static bool try_message(data_deserializer& s, uint8_t op, CB && callback) {
		if (op == T::op()) {
			T m;
			data_deserialize(s, m);
			callback(m);
			return true;
		}
		return false;
	}
};
