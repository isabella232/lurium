#pragma once

struct scorch_client {
	scorch_player* tank;
	point spectate_position;
	rect<int> view;
	bool paused;
	bool dead;
	bool sent_dead;

	scorch_client() {
		tank = NULL;
		spectate_position = point(-1, -1);
		view = rect<int>(0, 0, 0, 0);
		paused = false;
		dead = false;
		sent_dead = false;
	}
};

struct scorch_configuration {
	int width;
	int height;
	int pixels;
	int frame_rate;
	int max_players;
	int port;
	int min_bots;
	int max_bots;
	int seed;
	float gravity;
	float cooldown;
	float power;
	int terrain_height;
	int terrain_segment;
	std::string mode;
	std::string region;
	std::string description;
	std::string control_server_url;
	std::string control_promote_url;

	scorch_configuration() {
		width = 10000;
		height = 320;
		pixels = 4000;
		frame_rate = 30;
		max_players = 100;
		min_bots = 0;
		max_bots = 10;
		port = 9002;
		seed = 123;
		gravity = -0.666f; // TODO: 9.8 as default, convert from m/s to m/frame
		cooldown = 3.0;
		terrain_height = 200;
		terrain_segment = 50;
		power = 400;
		mode = "Default";
		description = "No description";
		region = "Unspecified";
	}
};

#include "lurium/host/game_server.h"

struct scorch_server : game_server<scorch_client> {
	scorch_engine game;
	int frame_rate;
	int view_radius;
	
	std::vector<scorch_bot> bots;
	scorch_configuration configuration;

	scorch_server(const scorch_configuration& c);

	void open_client(websocketpp::connection_hdl hdl, scorch_client& client);
	void close_client(websocketpp::connection_hdl hdl, scorch_client& client);
	void process_tick();

	//std::map<websocketpp::connection_hdl, scorch_client>::iterator get_client_by_player(const scorch_player& player);
	//void on_remove_player(const scorch_player& player);

	void on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg);
	void on_message_join(websocketpp::connection_hdl hdl, const scorch_message_join& m);
	void on_message_move(websocketpp::connection_hdl hdl, const scorch_message_move& m);
	void on_message_redraw(websocketpp::connection_hdl hdl, const scorch_message_redraw& m);
	void on_message_level_up(websocketpp::connection_hdl hdl, const scorch_message_level_up& m);
	void on_message_pause(websocketpp::connection_hdl hdl, const scorch_message_pause& m);
	void on_message_resume(websocketpp::connection_hdl hdl, const scorch_message_resume& m);

	void process_connection(websocketpp::connection_hdl hdl);
	void send_view(websocketpp::connection_hdl hdl, scorch_player* tank, const point& position, const rect<int>& prev_view, const rect<int>& view);
	void send_dead(websocketpp::connection_hdl hdl);
	void send_highscore(websocketpp::connection_hdl hdl);
	void send_clear(websocketpp::connection_hdl hdl);
	void send_joined(websocketpp::connection_hdl hdl);
	void send_full(websocketpp::connection_hdl hdl);
	void send_messages(websocketpp::connection_hdl hdl);

	void add_bot();
	void update_bots();
	void post_update_bots();

	//void run();
	//void stop();
};
