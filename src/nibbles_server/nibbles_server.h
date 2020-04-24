#pragma once

struct nibbles_client {
	int nibbles_id;
	point spectate_position;
	rect<int> view;

	nibbles_client() {
		nibbles_id = -1;
		spectate_position = point(-1, -1);
		view = rect<int>(0, 0, 0, 0);
	}
};

const int nibbles_protocol_join = 0;
const int nibbles_protocol_move = 1;
const int nibbles_protocol_ping = 2;
const int nibbles_protocol_view = 10;
const int nibbles_protocol_dead = 11;
const int nibbles_protocol_welcome = 12;
const int nibbles_protocol_highscore = 13;
const int nibbles_protocol_joined = 14;
const int nibbles_protocol_full = 15;
const int nibbles_protocol_pong = 16;

const int nibbles_protocol_object_pixel = 0;
const int nibbles_protocol_object_player = 1;

struct nibbles_configuration {
	int width;
	int height;
	int pixels;
	int frame_rate;
	int max_players;
	bool self_intersect;
	bool insta_death;
	int leave_remains;
	bool boost;
	int port;
	int min_bots;
	int max_bots;
	std::string mode;
	std::string region;
	std::string description;
	std::string control_server_url;
	std::string control_promote_url;

	nibbles_configuration() {
		width = 400;
		height = 400;
		pixels = 4000;
		frame_rate = 10;
		max_players = 100;
		min_bots = 0;
		max_bots = 10;
		port = 9002;
		self_intersect = false;
		insta_death = false;
		leave_remains = false;
		boost = false;
		mode = "Default";
		description = "No description";
		region = "Unspecified";
	}
};

#include "lurium/host/game_server.h"

struct nibbles_server : game_server<nibbles_client> {
	nibbles game;
	int frame_rate;
	std::vector<nibbles_bot> bots;
	nibbles_configuration configuration;

	nibbles_server(const nibbles_configuration& c);

	void open_client(websocketpp::connection_hdl hdl, nibbles_client& client);
	void close_client(websocketpp::connection_hdl hdl, nibbles_client& client);

	void process_tick();

	void on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg);
	void on_message_join(websocketpp::connection_hdl hdl, data_deserializer& des);
	void on_message_move(websocketpp::connection_hdl hdl, data_deserializer& des);
	void on_message_ping(websocketpp::connection_hdl hdl, data_deserializer& des);

	void process_connection(websocketpp::connection_hdl hdl);
	void send_view(websocketpp::connection_hdl hdl, const point& position, int radius, const rect<int>& prev_view, const rect<int>& view);
	void send_dead(websocketpp::connection_hdl hdl);
	void send_highscore(websocketpp::connection_hdl hdl);

	void add_bot();
	void update_bots();
	void post_update_bots();
	//void write_view_player(data_serializer& s, nibbles_player& player);
	//void write_view_pixel(data_serializer& s, nibbles_pixel& pixel);
};
