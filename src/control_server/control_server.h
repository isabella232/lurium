#pragma once

typedef websocketpp::server<websocketpp::config::asio> websocket_server;

struct control_protocol {
	std::string name;
	std::vector<std::string> api_keys;
};

struct control_configuration {
	std::vector<std::string> protocol_names;
	std::vector<control_protocol> protocols;
};

struct control_client {
	control_message_hello_v2 hello;
	control_message_status status;
/*	int version;
	int max_player_count;
	int player_count;
	std::string protocol; // nibbles, scorched
	std::string key;
	std::string url;
	std::string mode;
	std::string region;
	std::string description;*/
};

struct control_server {
	typedef std::map<websocketpp::connection_hdl, control_client, std::owner_less<websocketpp::connection_hdl>> client_map;

	websocket_server server;
	websocket_server::timer_ptr timer;
	client_map connections;

	control_server();
	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg);
	void on_http(websocketpp::connection_hdl hdl);
	void on_http_list(websocketpp::connection_hdl hdl, const std::string& query);
	void on_http_ip(websocketpp::connection_hdl hdl, const std::string& query);
	void on_pong_timeout(websocketpp::connection_hdl hdl, std::string s);
	void on_timer(const websocketpp::lib::error_code & ec);
	void on_hello(websocketpp::connection_hdl hdl, data_deserializer& reader);
	void on_status(websocketpp::connection_hdl hdl, data_deserializer& reader);
	void run(uint16_t port);
	void stop();
};
