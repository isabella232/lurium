#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include "lurium/host/data_serializer.h"
#include "lurium/host/websocket.h"
#include "websocket_impl.h"

websocket_handle_t websocket_create(const char* url, void* userdata, void(*onopen)(void*), void(*onclose)(void*), void(*onerror)(void*), void(*onmessage)(void*, const char*, int)) {
	auto handle = new websocket_impl(url, userdata);
	handle->on_open_callback = onopen;
	handle->on_close_callback = onclose;
	handle->on_error_callback = onerror;
	handle->on_message_callback = onmessage;
	handle->open();
	return handle;
}

void websocket_send(websocket_handle_t handle, const char* buffer, int length) {
	handle->send_binary(std::string(buffer, buffer + length));
}

void websocket_close(websocket_handle_t handle) {
	delete handle;
}
