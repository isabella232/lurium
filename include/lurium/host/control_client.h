#pragma once

typedef websocketpp::client<websocketpp::config::asio_client> websocket_client;

enum control_client_state {
	control_client_state_none,
	control_client_state_connecting,
	control_client_state_connected,
	control_client_state_failed,
	control_client_state_closed
};

struct control_client {
	control_message_hello_v2 hello;
	control_message_status status;
	std::string control_url;

	boost::atomic<control_client_state> state;
	websocket_client endpoint;
	websocketpp::connection_hdl handle;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;
	bool closing;
	websocket_client::timer_ptr reconnect_timer;
	websocket_client::timer_ptr status_timer;

	control_client(const std::string& _control_url, const std::string& protocol, const std::string& mode, const std::string& region, const std::string& url, const std::string& description, int max_players);
	~control_client();
	bool connect();
	void close();

	void set_status(int player_count);

	template <typename T>
	void send(T& m) {
		if (handle.use_count() == 0) {
			return;
		}

		std::string buffer;
		data_serializer s(buffer);

		s.write_uint8(T::op());
		data_serialize(s, m);

		websocketpp::lib::error_code ec;
		endpoint.send(handle, buffer, websocketpp::frame::opcode::binary, ec);
		if (ec) {
			std::cout << "on_open send failed: " << ec << " (" << ec.message() << ")" << std::endl;
		}
	}

	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_reconnect(const websocketpp::lib::error_code & ec);
	void on_status(const websocketpp::lib::error_code & ec);
};

