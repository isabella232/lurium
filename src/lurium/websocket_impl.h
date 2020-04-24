#pragma once

// Disable warning when truncating too long decorated typenames
#pragma warning(disable : 4503)

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "lurium/host/data_serializer.h"

typedef websocketpp::client<websocketpp::config::asio_client> websocket_endpoint;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

struct websocket_impl {
	std::string url;
	void* userdata;
	void(*on_open_callback)(void*);
	void(*on_close_callback)(void*);
	void(*on_error_callback)(void*);
	void(*on_message_callback)(void*, const char*, int);

	websocket_endpoint endpoint;
	websocketpp::connection_hdl handle;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

	websocket_impl(const std::string& _url, void* _userdata) {
		url = _url;
		userdata = _userdata;
		endpoint.clear_access_channels(websocketpp::log::alevel::all);
		endpoint.clear_error_channels(websocketpp::log::elevel::all);

		endpoint.init_asio();
		endpoint.start_perpetual();

		thread.reset(new websocketpp::lib::thread(&websocket_endpoint::run, &endpoint));
	}

	~websocket_impl() {
		close();
	}

	bool open() {
		websocketpp::lib::error_code ec;

		websocket_endpoint::connection_ptr con = endpoint.get_connection(url, ec);

		if (ec) {
			std::cout << "> Connect initialization error: " << ec.message() << std::endl;
			return false;
		}

		con->set_open_handler(bind(&websocket_impl::on_open, this, websocketpp::lib::placeholders::_1));
		con->set_close_handler(bind(&websocket_impl::on_close, this, websocketpp::lib::placeholders::_1));
		con->set_fail_handler(bind(&websocket_impl::on_fail, this, websocketpp::lib::placeholders::_1));
		con->set_message_handler(bind(&websocket_impl::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

		endpoint.connect(con);
		return true;
	}

	void close() {
		endpoint.stop_perpetual();

		if (handle.use_count() > 0) {
			websocketpp::lib::error_code ec;
			endpoint.close(handle, websocketpp::close::status::going_away, "", ec);
			if (ec) {
				std::cout << "Error closing connection: " << ec.message() << std::endl;
			}
		}

		thread->join();
	}

	void send_binary(const std::string& buffer) {
		websocketpp::lib::error_code ec;
		endpoint.send(handle, buffer, websocketpp::frame::opcode::binary, ec);

		if (ec) {
			std::cout << "Send failed: " << ec << " (" << ec.message() << ")" << std::endl;
		}
	}

	void on_open(websocketpp::connection_hdl hdl) {
		std::cout << "Connected to server." << std::endl;
		handle = hdl;
		if (on_open_callback) on_open_callback(userdata);
	}

	void on_close(websocketpp::connection_hdl hdl) {
		std::cout << "Disconnected from server." << std::endl;
		handle.reset();
		if (on_close_callback) on_close_callback(userdata);
	}

	void on_fail(websocketpp::connection_hdl hdl) {
		std::cout << "Failed connecting to server." << std::endl;
		if (on_error_callback) on_error_callback(userdata);
	}

	void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {

		if (msg->get_opcode() != websocketpp::frame::opcode::binary) {
			std::cout << "skipping non-binary frame" << std::endl;
			return;
		}

		const std::string payload = msg->get_payload();
		if (on_message_callback) on_message_callback(userdata, payload.c_str(), (int)payload.size());
	}
};
