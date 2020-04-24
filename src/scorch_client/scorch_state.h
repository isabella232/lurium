#pragma once

struct scorch_state {

	bool player_can_upgrade;
	int player_xp;
	glm::vec2 player_position; // TODO: actual player handle?
	std::vector<weapon_info> player_weapons;
	weapon_type player_weapon;
	std::queue<weapon_type> weapon_upgrades;
	std::vector<std::string> weapon_names;

	terrain_ground terrain;
	std::vector<scorch_player> players;
	std::vector<scorch_bullet> bullets;
	std::vector<scorch_explosion> explosions;
	std::vector<scorch_bonus> bonuses;

	int terrain_left;
	int terrain_width;
	int arena_width;
	int arena_height;
	glm::vec2 gravity;
	int tank_radius;
	int barrel_radius;

	float frame_rate;
	float power;
	float mouse_angle;
	bool space_pressed, a_pressed, d_pressed;

	websocket_client ws;

	std::function<void()> game_joined;
	std::function<void()> game_dead;
	std::function<void()> game_full;
	std::function<void()> game_welcome;
	std::function<void()> game_connected;
	std::function<void()> game_disconnected;
	std::function<void()> game_weapons_changed;
	std::function<void()> game_xp_changed;


	scorch_state()
		: terrain_left(0)
		, terrain_width(100)
		, tank_radius(8)
		, barrel_radius(10)
		, player_xp(0)
		, player_can_upgrade(false)
		, frame_rate(25.0f)
		, mouse_angle(0.0f)
		, power(500)
		, player_weapon(weapon_type::baby_missile)
		, space_pressed(false)
		, a_pressed(false)
		, d_pressed(false)
	{
	}

	void connect(const std::string& url) {
		ws.open(url.c_str());
	}

	void disconnect() {
		ws.close();
	}

	void join(const std::string& nick) {
		scorch_message_join m;
		m.nick = nick;
		ws.send(m);
	}

	void update_server() {
		// if input state was changed
		{
			scorch_message_move m;

			m.direction = mouse_angle;
			m.power = power;
			m.press_left = a_pressed;
			m.press_right = d_pressed;
			m.press_space = space_pressed;
			m.weapon = player_weapon;
			ws.send(m);
		}

		// TODO: callback for upgrades?? game view can says "ya, an upgrade was requested"
		if (!weapon_upgrades.empty()) {
			auto wep = weapon_upgrades.front();
			weapon_upgrades.pop();

			if (player_can_upgrade) {
				scorch_message_level_up m;
				m.weapon = wep;
				ws.send(m);
			}

		}
	}

	void poll() {
		ws.poll(
			std::bind(&scorch_state::process_message, this, std::placeholders::_1),
			std::bind(&scorch_state::socket_connected, this),
			std::bind(&scorch_state::socket_disconnected, this)
		);
	}

	void socket_connected() {
		if (game_connected) game_connected();
	}

	void socket_disconnected() {
		if (game_disconnected) game_disconnected();
	}

	void process_message(data_deserializer& s) {
		using x = websocket_client;
		uint8_t op;
		s.read(op);

		x::try_message<scorch_message_welcome>(s, op, std::bind(&scorch_state::on_welcome, this, std::placeholders::_1)) ||
			x::try_message<scorch_message_joined>(s, op, std::bind(&scorch_state::on_joined, this, std::placeholders::_1)) ||
			x::try_message<scorch_message_full>(s, op, std::bind(&scorch_state::on_full, this, std::placeholders::_1)) ||
			x::try_message<scorch_message_dead>(s, op, std::bind(&scorch_state::on_dead, this, std::placeholders::_1)) ||
			x::try_message<scorch_message_view>(s, op, std::bind(&scorch_state::on_view, this, std::placeholders::_1)) ||
			x::try_message<scorch_message_clear>(s, op, std::bind(&scorch_state::on_clear, this, std::placeholders::_1));
	}

	void on_welcome(scorch_message_welcome& m) {
		arena_height = m.height;
		arena_width = m.width;
		barrel_radius = m.barrel_radius;
		tank_radius = m.tank_radius;
		gravity = glm::vec2(0, m.gravity_y);

		frame_rate = m.framerate;
		weapon_names = m.weapon_names;
		//m.max_power;

		clear();
		player_weapons.clear(); // TODO: ??
		//weapon_buttons.update_weapons(m.weapon_names); // TOOD

		if (game_welcome) game_welcome();
		printf("onwelcome %i\n", m.framerate);// << std::endl;
	}

	void on_joined(scorch_message_joined& m) {
		printf("onjoined\n");
		clear();
		if (game_joined) game_joined();
	}

	void on_full(scorch_message_full& m) {
		if (game_full) game_full();
	}

	void on_dead(scorch_message_dead& m) {
		if (game_dead) game_dead();
	}

	void on_clear(scorch_message_clear& m) {
		printf("clear\n");
		clear();
	}

	void on_view(scorch_message_view& m) {
		//printf("onview\n");// << std::endl;

		auto scroll_distance = m.left - terrain_left;
		if (scroll_distance < 0) {
			if (-scroll_distance < terrain.ground.size()) {
				std::rotate(terrain.ground.begin(), terrain.ground.end() + scroll_distance, terrain.ground.end());
			}
		}
		else if (scroll_distance > 0) {
			if (scroll_distance < terrain.ground.size()) {
				std::rotate(terrain.ground.begin(), terrain.ground.begin() + scroll_distance, terrain.ground.end());
			}
		}

		terrain_width = m.width;
		terrain.ground.resize(m.width);
		terrain_left = m.left;

		if (player_xp != m.xp) {
			player_xp = m.xp;
			if (game_xp_changed) game_xp_changed();
			//weapon_buttons.request_redraw(); // TODO
			//game.dirty_hud = true;
		}

		// trigger redraw, and updating of button states, = dirty hud..
		bool can_upgrade = m.can_upgrade != 0;
		bool weapons_changed = false;
		if (player_can_upgrade != can_upgrade) {
			player_can_upgrade = can_upgrade;
			weapons_changed = true;
		}

		if (!m.weapons.empty()) {
			std::vector<weapon_info> weapons;
			for (auto& weapon : m.weapons) {
				weapon_info wep;
				wep.level = weapon.level;
				wep.max_cooldown = weapon.max_cooldown;
				wep.cooldown = weapon.cooldown;
				weapons.push_back(wep);
			}
			player_weapons = weapons;
			weapons_changed = true;
			//weapon_buttons.request_redraw();
		}

		if (weapons_changed) {
			if (game_weapons_changed) game_weapons_changed();
		}

		for (auto& section : m.ground_sections) {

			int index = section.left - terrain_left;
			for (auto& segments_list : section.segments) {
				if (index < 0 || index >= terrain_width) {
					continue;
				}
				scorch_ground ground;
				for (auto& segment : segments_list) {
					ground_segment g;
					g.y1 = segment.y1;
					g.y2 = segment.y2;
					g.falling = segment.falling != 0;
					g.fall_delay = segment.fall_delay;
					g.velocity = segment.velocity;
					ground.segments.push_back(g);
				}
				terrain.ground[index] = ground;
				index++;
			}
		}

		for (auto& player : m.insert_tanks) {
			//printf("insert player %i\n", player.object_id);
			insert_player(player.object_id, player.x, player.y, player.direction, player.centihealth, 100);
			if (player.x == m.x && player.y == m.y) {
				player_position = glm::vec2(m.x, m.y);
				//printf("insert self player\n");
			}
		}

		for (auto& player : m.update_tanks) {
			//printf("update player %i\n", player.object_id);
			update_player(player.object_id, player.x, player.y, player.direction, player.centihealth, 100);
			if (player.x == m.x && player.y == m.y) {
				player_position = glm::vec2(m.x, m.y);
				//printf("update self player\n");
			}
		}

		for (auto& player : m.delete_tanks) {
			//printf("remove player %i\n", player.object_id);
			delete_player(player.object_id);
		}

		for (auto& bullet : m.insert_bullets) {
			insert_bullet(bullet.object_id, bullet.x, bullet.y, bullet.velocity_x, bullet.velocity_y, bullet.has_gravity != 0, bullet.is_digger != 0, bullet.start_radius, bullet.end_radius, bullet.target_frames, bullet.frame);
		}

		assert(m.update_bullets.empty()); // bullets dont update

		for (auto& bullet : m.delete_bullets) {
			delete_bullet(bullet.object_id);
		}

		for (auto& explosion : m.insert_explosions) {
			insert_explosion(explosion.object_id, explosion.x, explosion.y, explosion.target_frames, explosion.target_radius, explosion.frame, explosion.explode_delay, explosion.is_dirt != 0);
		}

		assert(m.update_explosions.empty()); // explosions dont update

		for (auto& explosion : m.delete_explosions) {
			delete_explosion(explosion.object_id);
		}

		for (auto& bonus : m.insert_bonuses) {
			insert_bonus(bonus.object_id, bonus.x, bonus.y, (scorch_bonus_type)bonus.bonus_type);
		}

		assert(m.update_bonuses.empty()); // bonuses dont update

		for (auto& bonus : m.delete_bonuses) {
			delete_bonus(bonus.object_id);
		}


		tick_client();
	}

	void tick_client() {
		terrain.process(gravity.y);

		for (auto& weapon : player_weapons) {
			if (weapon.cooldown > 0) {
				weapon.cooldown--;
			}
		}

		for (auto& bullet : bullets) {
			process_bullet(bullet);
		}

		for (auto& explosion : explosions) {
			process_explosion(explosion);
		}
	}

	// TODO: scorch_client_engine and scorch_server_engine ..
	bool process_bullet(scorch_bullet& bullet) {

		auto position = bullet.position.add(bullet.velocity);

		if (bullet.has_gravity) {
			position = position.add(pointF(0, gravity.y));
		}

		if (bullet.is_digger) {
			auto radius_range = bullet.end_radius - bullet.start_radius;
			auto radius_unit = bullet.frame / (float)bullet.target_frames;
			auto bullet_radius = (int)(bullet.start_radius + radius_unit  * radius_range);
			terrain.modify_terrain(position.x - terrain_left, position.y, bullet_radius, false);

			if (bullet.frame >= bullet.target_frames) {
				return true;
			}
		}

		bullet.velocity = position.subtract(bullet.position);
		bullet.position = position;
		bullet.frame++;

		return false;
	}

	void process_explosion(scorch_explosion& explosion) {
		if (explosion.explode_delay > 0) {
			explosion.explode_delay--;
			return;
		}

		if (explosion.frame == explosion.target_frames - 1) {
			terrain.modify_terrain(explosion.position.x - terrain_left, explosion.position.y, explosion.target_radius, explosion.is_dirt);
		}
		explosion.frame++;
	}




	void insert_player(int object_id, int x, int y, float direction, int health, int max_health/*, const std::vector<weapon_info>& weapons*/) {
		scorch_player player;
		player.object_id = object_id;
		player.position.x = x;
		player.position.y = y;
		player.direction = direction;
		player.health = health;
		player.max_health = max_health;
		//player.weapons = weapons;
		players.push_back(player);
	}

	void update_player(int object_id, int x, int y, float direction, int health, int max_health/*, const std::vector<weapon_info>& weapons*/) {
		auto it = std::find_if(players.begin(), players.end(), [object_id](const scorch_player& u) {
			return u.object_id == object_id;
		});
		assert(it != players.end());

		scorch_player& player = *it;
		player.object_id = object_id;
		player.position.x = x;
		player.position.y = y;
		player.direction = direction;
		player.health = health;
		player.max_health = max_health;
		//player.weapons = weapons;
	}

	void delete_player(int object_id) {
		auto it = std::find_if(players.begin(), players.end(), [object_id](const scorch_player& u) {
			return u.object_id == object_id;
		});
		assert(it != players.end());

		scorch_player& player = *it;
		std::swap(player, players.back());
		players.pop_back();
	}

	void insert_bullet(int object_id, float x, float y, float velo_x, float velo_y, bool has_gravity, bool is_digger, int start_radius, int end_radius, int target_frames, int frame) {
		scorch_bullet bullet;
		bullet.object_id = object_id;
		bullet.position.x = x;
		bullet.position.y = y;
		bullet.velocity.x = velo_x;
		bullet.velocity.y = velo_y;
		bullet.has_gravity = has_gravity;
		bullet.is_digger = is_digger;
		bullet.start_radius = start_radius;
		bullet.end_radius = end_radius;
		bullet.target_frames = target_frames;
		bullet.frame = frame;
		bullets.push_back(bullet);
	}

	void delete_bullet(int object_id) {
		auto it = std::find_if(bullets.begin(), bullets.end(), [object_id](const scorch_bullet& u) {
			return u.object_id == object_id;
		});
		assert(it != bullets.end());

		scorch_bullet& bullet = *it;
		std::swap(bullet, bullets.back());
		bullets.pop_back();
	}

	void insert_explosion(int object_id, int x, int y, int target_frames, int target_radius, int frame, int explode_delay, bool is_dirt) {
		scorch_explosion explosion;
		explosion.object_id = object_id;
		explosion.position.x = x;
		explosion.position.y = y;
		explosion.target_frames = target_frames;
		explosion.target_radius = target_radius;
		explosion.frame = frame;
		explosion.explode_delay = explode_delay;
		explosion.is_dirt = is_dirt;
		explosions.push_back(explosion);
	}

	void delete_explosion(int object_id) {
		auto it = std::find_if(explosions.begin(), explosions.end(), [object_id](const scorch_explosion& u) {
			return u.object_id == object_id;
		});
		assert(it != explosions.end());

		scorch_explosion& explosion = *it;
		std::swap(explosion, explosions.back());
		explosions.pop_back();
	}

	void insert_bonus(int object_id, int x, int y, scorch_bonus_type type) {
		scorch_bonus bonus;
		bonus.object_id = object_id;
		bonus.position.x = x;
		bonus.position.y = y;
		bonus.bonus_type = type;
		bonuses.push_back(bonus);
	}

	void delete_bonus(int object_id) {
		auto it = std::find_if(bonuses.begin(), bonuses.end(), [object_id](const scorch_bonus& u) {
			return u.object_id == object_id;
		});
		assert(it != bonuses.end());

		scorch_bonus& bonus = *it;
		std::swap(bonus, bonuses.back());
		bonuses.pop_back();
	}

	void clear() {
		players.clear();
		bullets.clear();
		explosions.clear();
		bonuses.clear();
	}

};
