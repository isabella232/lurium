// Disable warning when truncating too long decorated typenames
#pragma warning(disable : 4503)

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/atomic.hpp>
#include <boost/lexical_cast.hpp>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "lurium/math/rand2.h"
#include "nibbles.h"
#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "lurium/host/control_client.h"
#include "nibbles_bot.h"
#include "nibbles_server.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

int index_from_point(double vx, double vy) {
	double avx = std::abs(vx);
	double avy = std::abs(vy);

	if (avx > avy) {
		if (vx > 0) {
			return 0;
		} else if (vx < 0) {
			return 2;
		}
	} else if (avx < avy) {
		if (vy > 0) {
			return 1;
		} else if (vy < 0) {
			return 3;
		}
	}

	assert(avx == avy);

	// check which edge remains, and return clock wise additional. interpret edges like so:
	//  wNn
	//  WXE
	//  sSe

	if (vx > 0 && vy < 0) {
		return 0;
	} else if (vy < 0 && vx < 0) {
		return 1;
	} else if (vx < 0 && vy > 0) {
		return 2;
	} else if (vy > 0 && vx > 0) {
		return 3;
	}

	// only 0,0 remains, lets say its q0 (right/east)
	return 0;
}

std::string get_config_string(const nibbles_configuration& config) {
	std::stringstream description;
	description << "Width/Height: " << config.width << "x" << config.height << " px" << std::endl;
	description << "Frame Rate: " << config.frame_rate << " frames/sec" << std::endl;
	description << "Self Intersection: " << (config.self_intersect ? "Yes" : "No") << std::endl;
	description << "Insta Death: " << (config.insta_death ? "Yes" : "No") << std::endl;
	description << "Leave Remains: " << (config.leave_remains ? "Yes" : "No") << std::endl;
	description << "Boost Enabled: " << (config.boost ? "Yes" : "No") << std::endl;
	return description.str();
}

nibbles_server::nibbles_server(const nibbles_configuration& c) 
	: game(c.width, c.height, c.pixels, c.pixels * 2, c.max_players, c.self_intersect, c.insta_death, c.leave_remains, c.boost)
	, game_server(c.frame_rate, c.port, c.control_server_url, "nibbles", c.mode, c.region, c.control_promote_url, get_config_string(c), c.max_players)
{
	configuration = c;
	frame_rate = configuration.frame_rate;

	server.set_message_handler(bind(&nibbles_server::on_message, this, ::_1, ::_2));

	closing = false;

	for (int i = 0; i < configuration.max_bots; ++i) {
		add_bot();
	}
}

void nibbles_server::open_client(websocketpp::connection_hdl hdl, nibbles_client& client) {
	websocketpp::lib::error_code ec;

	client.nibbles_id = -1;
	client.spectate_position = point(game.width / 2, game.height / 2);
	client.view = rect<int>(-1, -1, -1, -1);
	//connections[hdl] = client;

	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(nibbles_protocol_welcome);
	s.write_uint16(game.width);
	s.write_uint16(game.height);
	s.write_uint8(frame_rate);

	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);

	if (ec) {
		std::cout << "on_open send failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void nibbles_server::close_client(websocketpp::connection_hdl hdl, nibbles_client& client) {
}

void nibbles_server::on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg) {

	if (msg->get_opcode() != websocketpp::frame::opcode::binary) {
		std::cout << "skipping non-binary frame" << std::endl;
		return;
	}

	data_deserializer des(msg->get_payload());
	uint8_t code;
	des.read(code);

	switch (code) {
		case nibbles_protocol_join:
			on_message_join(hdl, des);
			break;
		case nibbles_protocol_move:
			on_message_move(hdl, des);
			break;
		case nibbles_protocol_ping:
			on_message_ping(hdl, des);
			break;
		default:
			break;
	}
}

void nibbles_server::send_highscore(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	nibbles_client& client = connections[hdl];
	if (client.nibbles_id == -1) {
		return;
	}

	std::vector<nibbles_player> result;

	for (auto& p : game.players) {
		if (!p.active) {
			continue;
		}

		result.push_back(p);
		std::sort(result.begin(), result.end(),
			[](nibbles_player const& a, nibbles_player const& b) { return a.length > b.length; });

		if (result.size() > 10)
			result.erase(result.begin() + 10, result.end());
	}


	std::string buffer;
	data_serializer s(buffer);
	s.write_uint8(nibbles_protocol_highscore);
	s.write_uint16((uint16_t)result.size());
	for (auto& p : result) {
		s.write_string(p.nick);
		s.write_uint32(p.length);
	}

	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "send_hello() failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void write_view_player(data_serializer& s, nibbles_player& player, diff_modify_type type) {

	s.write_uint8(nibbles_protocol_object_player);
	s.write_uint32(player.object_id);

	if (type == diff_modify_type_remove) {
		return;
	}

	s.write_uint8(player.color);
	s.write_uint16(player.position.x);
	s.write_uint16(player.position.y);

	s.write_uint16(player.segments.size());
	for (auto p : player.segments) {
		s.write_uint8(p.direction);
		s.write_uint16(p.length);
	}

	if (type == diff_modify_type_add) {
		s.write_string(player.nick);
	} else {
		s.write_string("");
	}
}

void write_view_pixel(data_serializer& s, nibbles_pixel& pixel, diff_modify_type type) {
	s.write_uint8(nibbles_protocol_object_pixel);
	s.write_uint32(pixel.object_id);
	if (type == diff_modify_type_remove) {
		return;
	}

	s.write_uint8(pixel.color);
	s.write_uint16(pixel.position.x);
	s.write_uint16(pixel.position.y);
}


void nibbles_server::process_connection(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	nibbles_client& client = connections[hdl];

	if (client.nibbles_id == -1) {
		const point& position = client.spectate_position;
		int radius = radius = 100 * 0.2 + 30;;
		rect<int> view(
			std::max(position.x - radius, 0),
			std::max(position.y - radius, 0),
			std::min(position.x + radius, game.width),
			std::min(position.y + radius, game.height));

		send_view(hdl, client.spectate_position, radius, client.view, view);
		client.view = view;
	} else {

		nibbles_player& this_player = game.players[client.nibbles_id];

		if (!this_player.active && this_player.deactivate_timestamp == game.timestamp) {
			// release to another client, emit death message
			client.nibbles_id = -1;
			client.spectate_position = this_player.position;
			send_dead(hdl);
		}

		int max_radius_size = std::min((int)this_player.length, 100);
		int radius = max_radius_size * 0.2 + 30;

		rect<int> view(
			std::max(this_player.position.x - radius, 0),
			std::max(this_player.position.y - radius, 0),
			std::min(this_player.position.x + radius, game.width),
			std::min(this_player.position.y + radius, game.height));

		send_view(hdl, this_player.position, radius, client.view, view);

		client.view = view;
	}
}

void nibbles_server::send_dead(websocketpp::connection_hdl hdl) {
	// TODO: send stats like max length, dead length, kills?

	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(nibbles_protocol_dead);

	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);

	if (ec) {
		std::cout << "send_dead failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void nibbles_server::send_view(websocketpp::connection_hdl hdl, const point& position, int radius, const rect<int>& prev_view, const rect<int>& view) {
	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(nibbles_protocol_view);
	s.write_uint16(position.x);
	s.write_uint16(position.y);

	// determine scale/soom based on length (could use bounding boks as well - curled up = see less)
	s.write_uint16(radius);

	// write diff
	std::vector<diff_object<nibbles_base*>> diff;
	game.collider.query_diff(prev_view, view, diff);

	s.write_uint16(diff.size());
	for (auto& object : diff) {
		s.write_uint8(object.state);

		switch (object.data->type) {
		case nibbles_object_type_pixel:
			write_view_pixel(s, (nibbles_pixel&)*object.data, object.state);
			break;
		case nibbles_object_type_player:
			write_view_player(s, (nibbles_player&)*object.data, object.state);
			break;
		default:
			assert(false);
		}
	}

	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "send_view failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void nibbles_server::process_tick() {
	update_bots();

	game.tick();

	auto highscore_duration = system_clock::now() - highscore_time;
	auto highscore_ms = boost::chrono::duration_cast<boost::chrono::milliseconds>(highscore_duration).count();

	for (auto& x : this->connections) {
		process_connection(x.first);
		if (highscore_ms > 1000) {
			send_highscore(x.first);
			highscore_time = system_clock::now();
		}
	}

	// purge dead bots
	post_update_bots();

	// collider diff transmitted, update diffing quadtree and advance game time
	game.tick_done();

	control.set_status(game.active_players);

}

void nibbles_server::on_message_join(websocketpp::connection_hdl hdl, data_deserializer& des) {

	std::string nick;
	des.read(nick);

	nibbles_client& client = connections[hdl];
	client.nibbles_id = game.spawn_player(nick);
	client.view = rect<int>(-1, -1, -1, -1);

	std::string buffer;
	data_serializer s(buffer);

	if (client.nibbles_id != -1) {
		s.write_uint8(nibbles_protocol_joined);
	} else {
		s.write_uint8(nibbles_protocol_full);
	}

	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);
	
	if (ec) {
		std::cout << "on_message_join failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void nibbles_server::on_message_move(websocketpp::connection_hdl hdl, data_deserializer& des) {
	nibbles_client& client = connections[hdl];
	if (client.nibbles_id == -1) {
		return;
	}

	uint8_t inbits;
	des.read(inbits);

	int direction = inbits & 3;
	bool press_space = (inbits & 4) != 0;

	nibbles_player& this_player = game.players[client.nibbles_id];
	game.set_input(this_player, direction, press_space);
}

void nibbles_server::on_message_ping(websocketpp::connection_hdl hdl, data_deserializer& des) {
	std::string token;
	des.read(token);

	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(nibbles_protocol_pong);
	s.write_string(token);
	
	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "on_message_ping() failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void nibbles_server::add_bot() {
	bots.push_back(nibbles_bot());
}

const std::vector<std::string> botNames = { "Notabot", "Nobot", "100%HUMAN", "HAL 9000", "onlyhuman", "RealPerson", "Hubot", "JOHNSMITH", "AI lmao", "WOPR", "RIPLEY", "Bender", "Data", "Lore", "TheBorgCollective", "Locutus", "Seven", "Marvin", "Robby", "Ash", "T-800", "Fembot" };

void nibbles_server::update_bots() {
	int active_bots = 0;
	for (auto& bot : bots) {
		if (bot.client_id != -1) {
			active_bots++;
		}
	}

	// threshold spawning of bots through min/max config and number of human players
	int active_humans = game.active_players - active_bots;
	int target_bots = std::max(configuration.max_bots - active_humans, configuration.min_bots);

	for (auto& bot : bots) {
		if (bot.client_id == -1 && active_bots < target_bots) {
			bot.client_id = game.spawn_player(botNames[rand2(botNames.size())]);
			if (bot.client_id != -1) {
				active_bots++;
			}
		}

		bot.update(game);
	}
}

void nibbles_server::post_update_bots() {
	for (auto& bot : bots) {
		bot.post_update(game);
	}
}
