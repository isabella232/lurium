#define _USE_MATH_DEFINES

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
#include "lurium/math/random.h"
#include "ground.h"
#include "scorch.h"
#include "scorch_bot.h"
#include "lurium/host/data_serializer.h"
#include "lurium/host/control_message.h"
#include "lurium/host/control_client.h"
#include "lurium/host/data_serializer.h"
#include "scorch_message.h"
#include "scorch_server.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

std::string get_config_string(const scorch_configuration& config) {
	std::stringstream description;
	description << "Width/Height: " << config.width << "x" << config.height << " px" << std::endl;
	description << "Frame Rate: " << config.frame_rate << " frames/sec" << std::endl;
	return description.str();
}

inline float get_cooldown_rate(int frame_rate, float sec) {
	return 1.0f / (sec * frame_rate);
}

inline float get_gravity(int frame_rate, float gravity) {
	return gravity / frame_rate;
}

inline float get_power(int frame_rate, float power) {
	return power / frame_rate;
}

scorch_server::scorch_server(const scorch_configuration& c)
	: game(c.width, c.height, c.frame_rate, c.max_players, 20, 200, c.width / 100, c.seed, c.terrain_height, c.terrain_segment, get_gravity(c.frame_rate, c.gravity), get_cooldown_rate(c.frame_rate, c.cooldown), get_power(c.frame_rate, c.power))
	, game_server(c.frame_rate, c.port, c.control_server_url, "scorch", c.mode, c.region, c.control_promote_url, get_config_string(c), c.max_players)
{
	configuration = c;
	frame_rate = configuration.frame_rate;
	view_radius = 320;

	server.set_message_handler(bind(&scorch_server::on_message, this, ::_1, ::_2));

	closing = false;

	//game.on_remove_player = std::bind(&scorch_server::on_remove_player, this, _1);
	//game.generate_ground();

	for (int i = 0; i < configuration.max_bots; ++i) {
		add_bot();
	}
}

void scorch_server::open_client(websocketpp::connection_hdl hdl, scorch_client& client) {
	client.tank = NULL;
	client.spectate_position = point(game.width / 2, game.height / 2);
	client.view = rect<int>(-1, -1, -1, -1);
	client.paused = false;
	client.dead = false;
	client.sent_dead = false;

	scorch_message_welcome msg;
	msg.version = 0xFD;
	msg.width = game.width;
	msg.height = game.height;
	msg.framerate = frame_rate;
	msg.tank_radius = game.TANK_RADIUS;
	msg.barrel_radius = game.BARREL_RADIUS;
	msg.gravity_y = game.gravity.y;
	msg.max_power = game.max_power;

	// TODO: send max power (ratio)

	for (int i = 0; i < max_weapons; i++) {
		msg.weapon_names.push_back(weapon_type_name[i]);
	}

	send(hdl, msg);
}

void scorch_server::close_client(websocketpp::connection_hdl hdl, scorch_client& client) {
	if (client.tank != NULL) {
		game.remove_player(*client.tank);
	}
}

void scorch_server::send_clear(websocketpp::connection_hdl hdl) {
	send(hdl, scorch_message_clear());
}

void scorch_server::send_joined(websocketpp::connection_hdl hdl) {
	send(hdl, scorch_message_joined());
}

void scorch_server::send_full(websocketpp::connection_hdl hdl) {
	send(hdl, scorch_message_full());
}

void scorch_server::send_highscore(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	scorch_client& client = connections[hdl];
	if (client.tank == NULL) {
		return;
	}

	std::vector<scorch_player> result;

	for (auto& p : game.tanks) {
		if (!p.active) {
			continue;
		}

		result.push_back(p);
		std::sort(result.begin(), result.end(),
			[](scorch_player const& a, scorch_player const& b) { return a.xp > b.xp; });

		if (result.size() > 10)
			result.erase(result.begin() + 10, result.end());
	}

	/*
	std::string buffer;
	data_serializer s(buffer);
	s.write_uint8(scorch_protocol_highscore);
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
	*/
}
/*
std::map<websocketpp::connection_hdl, scorch_client>::iterator scorch_server::get_client_by_player(const scorch_player& player) {
	return find_if(connections.begin(), connections.end(), [&player](const std::pair<websocketpp::connection_hdl, scorch_client>& v) {
		return v.second.tank == &player;
	});
}

void scorch_server::on_remove_player(const scorch_player& player) {
	auto c = get_client_by_player(player);
	if (c == connections.end()) {
		// bots and disconnected players dont have clients
		return;
	}

	websocketpp::connection_hdl hdl = c->first;
	scorch_client& client = c->second;

	client.tank = NULL;// .scorch_id = -1;
	client.spectate_position = player.position;
	
	// TODO: if paused, dont send death until resumed
	// 
	send_dead(hdl);

	// TODO: take killing player as parameter too, generate messages here; kan andre ting kill?
	// TODO: this is only place doing socket stuff in game engine tick; should really poll these!
}*/


void scorch_server::process_connection(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	scorch_client& client = connections[hdl];

	// check for death, clear client tank; assumes engine sets dead=true and leaves tank active at least until following frame
	if (client.tank != NULL && client.tank->dead) {
		client.spectate_position = client.tank->position;
 		client.tank = NULL;
		client.dead = true;
		client.sent_dead = false;
	}

	// check for paused, ie dont send dead until resumed
	if (client.paused) {
		// e.g user is in a different browser tab, or mobile app is in the background
		return;
	}

	if (client.dead && !client.sent_dead) {
		send_dead(hdl);
		client.sent_dead = true;
	}

	if (client.tank == NULL) {

		const point& position = client.spectate_position;

		auto view = rect<int>(position.x - view_radius, 0, position.x + view_radius, game.height);
		send_view(hdl, NULL, position, client.view, view);
		client.view = view;
	} else {

		scorch_player& this_player = *client.tank;

		if (!this_player.messages.empty()) {
			send_messages(hdl);
		}

		/*
		if (!this_player.active && this_player.deactivate_timestamp == game.timestamp) {
			// release to another client, emit death message
			client.tank = NULL;// .scorch_id = -1;
			client.spectate_position = this_player.position;
			send_dead(hdl);
		}*/

		auto view = rect<int>(this_player.position.x - view_radius, 0, this_player.position.x + view_radius, game.height);

		send_view(hdl, client.tank, this_player.position, client.view, view);

		client.view = view;
	}
}

void scorch_server::send_dead(websocketpp::connection_hdl hdl) {
	// TODO: send stats like max length, dead length, kills?
	send(hdl, scorch_message_dead());
}

void scorch_server::send_messages(websocketpp::connection_hdl hdl) {
	auto con = server.get_con_from_hdl(hdl);

	scorch_client& client = connections[hdl];

	if (client.tank == NULL) {
		return;
	}

	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(scorch_protocol_message);
	s.write_uint8(client.tank->messages.size());

	for (const auto& message : client.tank->messages) {
		s.write_uint8(message.type);
		s.write_string(message.name);
	}

	websocketpp::lib::error_code ec;
	server.send(hdl, buffer, websocketpp::frame::opcode::binary, ec);

	if (ec) {
		std::cout << "send_messages failed: " << ec << " (" << ec.message() << ")" << std::endl;
	}
}

void write_view_bullet(data_serializer& s, scorch_bullet& object, diff_modify_type type) {
	s.write_uint8(nibbles_protocol_object_bullet);
	s.write_uint32(object.object_id);
	if (type == diff_modify_type_remove) {
		return;
	}
	s.write_float(object.position.x);
	s.write_float(object.position.y);
	s.write_float(object.velocity.x);
	s.write_float(object.velocity.y);

	s.write_uint8(object.has_gravity ? 1 : 0);
	s.write_uint8(object.is_digger ? 1 : 0);
	s.write_uint8(object.start_radius);
	s.write_uint8(object.end_radius);
	s.write_uint16(object.target_frames);
	s.write_uint16(object.frame);
	// TODO: obey gravity, weapontype, start/end radius, duration, frame, digging +/- leaves falling for server/client sync of different bullet types
}

void write_view_explosion(data_serializer& s, scorch_explosion& object, diff_modify_type type) {
	s.write_uint8(nibbles_protocol_object_explosion);
	s.write_uint32(object.object_id);
	if (type == diff_modify_type_remove) {
		return;
	}
	s.write_int32(object.position.x);
	s.write_int32(object.position.y);
	s.write_uint16(object.target_frames);
	s.write_uint16(object.target_radius);
	s.write_uint16(object.frame);
	s.write_uint16(object.explode_delay);
	s.write_uint8(object.is_dirt ? 1 : 0);
}

void write_view_player(data_serializer& s, scorch_player& player, diff_modify_type type) {
	s.write_uint8(nibbles_protocol_object_player);
	s.write_uint32(player.object_id);
	if (type == diff_modify_type_remove) {
		return;
	}
	s.write_uint8(player.color);
	s.write_uint16(player.position.x);
	s.write_uint16(player.position.y);
	s.write_float(player.direction);
	s.write_uint16(player.power * 1000.0);

	auto centihealth = ((float)player.health / player.max_health) * 100.0f;
	centihealth = std::max(centihealth, 0.0f);
	s.write_uint16(centihealth);

	if (type == diff_modify_type_add) {
		s.write_string(player.name);
	} else {
		s.write_string("");
	}
}

void write_view_bonus(data_serializer& s, scorch_bonus& object, diff_modify_type type) {
	s.write_uint8(nibbles_protocol_object_bonus);
	s.write_uint32(object.object_id);
	if (type == diff_modify_type_remove) {
		return;
	}
	s.write_int32(object.position.x);
	s.write_int32(object.position.y);
	s.write_uint16(object.bonus_type);
}

void scorch_server::send_view(websocketpp::connection_hdl hdl, scorch_player* tank, const point& position, const rect<int>& prev_view, const rect<int>& view) {

	std::string buffer;
	data_serializer s(buffer);

	s.write_uint8(scorch_protocol_view);
	s.write_uint16(position.x);
	s.write_uint16(position.y);

	int width = view.right - view.left;
	s.write_int32(view.left);
	s.write_uint16(width);

	if (tank != NULL) {
		// TODO: cooldown always 0..100, + integer rate
		//s.write_uint8(tank->cooldown * 100);
		s.write_uint32(tank->xp);
		auto can_upgrade = (tank->upgrades < game.get_tank_level(*tank)) ? 1 : 0;
		s.write_uint8(can_upgrade);
	} else {
		s.write_uint32(0);
		s.write_uint8(0);
	}


	// send weapons_count and weapon only if dirty_weapon (and obvously only when tank != NULL)
	std::vector<scorch_message_view::tank_weapon> weapons;
	if (tank != NULL && tank->dirty_weapon) {
		for (auto w : tank->weapons) {
			scorch_message_view::tank_weapon mw;
			mw.level = w.level;
			mw.cooldown = w.cooldown;
			mw.max_cooldown = w.max_cooldown;

			// can upgrade = hvis vi har unspent upgrade points?
			// hvis prisen er den samme, så er det global.. som diep
			// 1 upgrade point = 1 level -> ikke som diep. .
			// eller skal vi ha cost pr weapon pr level, trade xp for våpen?

			weapons.push_back(mw);
		}
		/*s.write_uint8(tank->weapon_counts.size());
		s.write_uint8(tank->weapon);
		for (auto count : tank->weapon_counts) {
			s.write_uint8(count);
		}
		s.write_uint8(0);*/
	} else {
		//s.write_uint8(0);
	}
	data_serialize(s, weapons);

	std::vector<rect<int> > new_rects;
	new_rects.push_back(view);
	rect<int>::subtract(new_rects, prev_view);

	// transmit ground at wave position! if in view, (todo: client side collider object w/position)
	// if wave x is not in new_rects, add it - this is quickndirty fix until it be animated
	if (game.ground_wave_counter >= 0 && view.contains(game.ground_wave_counter, view.top)) {
		bool has_wave = false;
		for (const auto& r : new_rects) {
			if (r.contains(game.ground_wave_counter, r.top)) {
				has_wave = true;
				break;
			}
		}
		if (!has_wave) {
			new_rects.push_back(rect<int>(game.ground_wave_counter, view.top, game.ground_wave_counter + 1, view.bottom));
		}
	}

	s.write_uint16(new_rects.size());
	for (auto& r : new_rects) {
		s.write_int32(r.left);
		s.write_int32(r.right);
		for (int i = r.left; i < r.right; i++) {
			if (i < 0 || i >= game.terrain.ground.size()) {
				s.write_uint16(0);
			} else {
				auto& ground = game.terrain.ground[i];
				s.write_uint16(ground.segments.size());
				for (auto& segment : ground.segments) {
					s.write_float(segment.y1);
					s.write_float(segment.y2);
					s.write_uint8(segment.falling ? 1 : 0);
					s.write_uint16(segment.fall_delay);
					s.write_float(segment.velocity);
				}
			}
		}
	}

	auto query_view = view;
	query_view.constrict_bound(0, 0, game.width, game.height);

	auto query_prev_view = prev_view;
	if (prev_view.left != prev_view.right && prev_view.top != prev_view.bottom) {
		query_prev_view.constrict_bound(0, 0, game.width, game.height);
	}
/*
	auto query_prev_view = prev_view;
	query_prev_view.constrict_bound(0, 0, game.width, game.height);
	*/
	// write diff
	std::vector<diff_object<scorch_object*> > diff;
	game.query_diff(query_prev_view, query_view, diff);

	// TOOD: vi får isnerts og deletes her, og updates på alle players siden has_changed er true! skal bare ha update når de nedrer angle, .. 

	s.write_uint16(diff.size());
	for (auto& object : diff) {
		s.write_uint8(object.state);

		switch (object.data->type) {
			case scorch_object_type_bullet:
				write_view_bullet(s, (scorch_bullet&)*object.data, object.state);
				break;
			case scorch_object_type_explosion:
				write_view_explosion(s, (scorch_explosion&)*object.data, object.state);
				break;
			case scorch_object_type_tank:
				write_view_player(s, (scorch_player&)*object.data, object.state);
				break;
			case scorch_object_type_bonus:
				write_view_bonus(s, (scorch_bonus&)*object.data, object.state);
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

void scorch_server::process_tick() {
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


void scorch_server::on_message(websocketpp::connection_hdl hdl, websocket_server::message_ptr msg) {

	if (msg->get_opcode() != websocketpp::frame::opcode::binary) {
		std::cout << "skipping non-binary frame" << std::endl;
		return;
	}

	data_deserializer des(msg->get_payload());
	uint8_t code;
	des.read(code);

	try_message<scorch_message_join>(hdl, des, code, std::bind(&scorch_server::on_message_join, this, _1, _2)) ||
		try_message<scorch_message_move>(hdl, des, code, std::bind(&scorch_server::on_message_move, this, _1, _2)) ||
		try_message<scorch_message_redraw>(hdl, des, code, std::bind(&scorch_server::on_message_redraw, this, _1, _2)) ||
		try_message<scorch_message_level_up>(hdl, des, code, std::bind(&scorch_server::on_message_level_up, this, _1, _2)) ||
		try_message<scorch_message_pause>(hdl, des, code, std::bind(&scorch_server::on_message_pause, this, _1, _2)) ||
		try_message<scorch_message_resume>(hdl, des, code, std::bind(&scorch_server::on_message_resume, this, _1, _2))
		;

}

void scorch_server::on_message_join(websocketpp::connection_hdl hdl, const scorch_message_join& m) {

	// TODO: can we queue the join request so that its processed in tick(), and send joined after a callback
	// or rearrange tick(), such that spawns are processed in tick_done()

	auto tank = game.spawn_player(m.nick);

	scorch_client& client = connections[hdl];
	client.view = rect<int>(-1, -1, -1, -1);

	if (tank != game.tanks.end()) {
		client.tank = &*tank;
		send_joined(hdl);
	} else {
		client.tank = NULL;
		send_full(hdl);
	}
}

void scorch_server::on_message_move(websocketpp::connection_hdl hdl, const scorch_message_move& m) {
	scorch_client& client = connections[hdl];
	if (client.tank == NULL) {
		return;
	}

	weapon_type weapon = (weapon_type)m.weapon;

	scorch_player& this_player = *client.tank;// game.tanks[client.scorch_id];
	this_player.direction = m.direction;
	this_player.power = (float)m.power / 1000.0f;
	this_player.press_space = m.press_space != 0;
	this_player.press_left = m.press_left != 0;
	this_player.press_right = m.press_right != 0;

	if (this_player.weapon != weapon) {
		this_player.weapon = weapon;
		this_player.dirty_weapon = true;
	}
}

void scorch_server::on_message_redraw(websocketpp::connection_hdl hdl, const scorch_message_redraw& m) {
	scorch_client& client = connections[hdl];
	if (client.tank == NULL) {
		return;
	}
	client.view = rect<int>(-1, -1, -1, -1);

	send_clear(hdl);
}

void scorch_server::on_message_level_up(websocketpp::connection_hdl hdl, const scorch_message_level_up& m) {
	scorch_client& client = connections[hdl];
	if (client.tank == NULL) {
		return;
	}

	auto level = game.get_tank_level(*client.tank);
	if (client.tank->upgrades < level) {
		client.tank->weapons[m.weapon].level++;
		client.tank->dirty_weapon = true;
		client.tank->upgrades++;
	}
}

void scorch_server::on_message_pause(websocketpp::connection_hdl hdl, const scorch_message_pause& m) {
	scorch_client& client = connections[hdl];
	client.paused = true;
}

void scorch_server::on_message_resume(websocketpp::connection_hdl hdl, const scorch_message_resume& m) {
	scorch_client& client = connections[hdl];
	if (!client.paused) {
		return;
	}

	client.paused = false;
	client.view = rect<int>(-1, -1, -1, -1);
	send_clear(hdl);
}

void scorch_server::add_bot() {
	bots.push_back(scorch_bot());
}

const std::vector<std::string> botNames = { "Notabot", "Nobot", "100%HUMAN", "HAL 9000", "onlyhuman", "RealPerson", "Hubot", "JOHNSMITH", "AI lmao", "WOPR", "RIPLEY", "Bender", "Data", "Lore", "TheBorgCollective", "Locutus", "Seven", "Marvin", "Robby", "Ash", "T-800", "Fembot" };

//extern int rand2(int range);

void scorch_server::update_bots() {
	int active_bots = 0;
	for (auto& bot : bots) {
		if (bot.tank != NULL) {
			active_bots++;
		}
	}

	// threshold spawning of bots through min/max config and number of human players
	int active_humans = game.active_players - active_bots;
	int target_bots = std::max(configuration.max_bots - active_humans, configuration.min_bots);

	for (auto& bot : bots) {
		if (bot.tank == NULL && active_bots < target_bots) {
			auto tank = game.spawn_player(botNames[rand2(botNames.size())]);
			if (tank != game.tanks.end()) {
				bot.tank = &*tank;
				active_bots++;
			}
		}

		bot.update(game);
	}
}

void scorch_server::post_update_bots() {
	for (auto& bot : bots) {
		bot.post_update(game);
	}
}

