#pragma once

const int scorch_protocol_welcome = 0;
const int scorch_protocol_join = 1;
const int scorch_protocol_dead = 2;
const int scorch_protocol_view = 3;
const int scorch_protocol_joined = 4;
const int scorch_protocol_full = 5;
const int scorch_protocol_ping = 6;
const int scorch_protocol_pong = 7;
const int scorch_protocol_move = 8;
const int scorch_protocol_redraw = 9;
const int scorch_protocol_clear = 10;
const int scorch_protocol_message = 11;
const int scorch_protocol_level_up = 12;
const int scorch_protocol_pause = 13;
const int scorch_protocol_resume = 14;

const int nibbles_protocol_object_bullet = 0;
const int nibbles_protocol_object_player = 1;
const int nibbles_protocol_object_explosion = 2;
const int nibbles_protocol_object_bonus = 3;

struct scorch_message_welcome : data_message<scorch_protocol_welcome> {
	uint8_t version;
	uint16_t width;
	uint16_t height;
	uint8_t framerate;

	uint8_t tank_radius;
	uint8_t barrel_radius;
	float gravity_y;
	float max_power;
	std::vector<std::string> weapon_names;
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_welcome& m) {
	s.write(m.width);
	s.write(m.height);
	s.write(m.framerate);

	s.write(m.tank_radius);
	s.write(m.barrel_radius);
	s.write(m.gravity_y);

	// NOTE/TODO: currently live protocol uses uint8 for max_weapons; this breaks it!
	s.write_uint8((uint8_t)m.weapon_names.size());
	for (auto n : m.weapon_names) {
		s.write(n);
	}
	//data_serialize(s, m.weapon_names);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_welcome& m) {
	s.read(m.width);
	s.read(m.height);
	s.read(m.framerate);

	s.read(m.tank_radius);
	s.read(m.barrel_radius);
	s.read(m.gravity_y);

	uint8_t len;
	s.read(len);
	m.weapon_names.clear();
	for (int i = 0; i < len; i++) {
		std::string n;
		s.read(n);
		m.weapon_names.push_back(n);
	}
	//data_deserialize(s, m.weapon_names);
}

struct scorch_message_clear : data_message<scorch_protocol_clear> {};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_clear& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_clear& m) {}


struct scorch_message_joined : data_message<scorch_protocol_joined> {};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_joined& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_joined& m) {}

struct scorch_message_full : data_message<scorch_protocol_full> {};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_full& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_full& m) {}


struct scorch_message_dead : data_message<scorch_protocol_dead> {};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_dead& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_dead& m) {}

struct scorch_message_view : data_message<scorch_protocol_view> {

	struct ground_segment {
		float y1, y2;
		uint8_t falling;
		uint16_t fall_delay;
		float velocity;
	};

	struct ground_section {
		int32_t left;
		int32_t right;
		std::vector<std::vector<ground_segment>> segments;
	};

	struct insert_update_tank {
		uint32_t object_id;
		uint8_t color;
		uint16_t x, y;
		float direction;
		uint16_t power;
		uint16_t centihealth;
		uint16_t shield;
		std::string name;
	};

	struct insert_update_bullet {
		uint32_t object_id;
		float x, y;
		float velocity_x, velocity_y;
		uint8_t has_gravity;
		uint8_t is_digger;
		uint8_t start_radius;
		uint8_t end_radius;
		uint16_t target_frames;
		uint16_t frame;
	};

	struct insert_update_explosion {
		uint32_t object_id;
		int32_t x, y;
		uint16_t target_frames;
		uint16_t target_radius;
		uint16_t frame;
		uint16_t explode_delay;
		uint8_t is_dirt;
	};

	struct insert_update_bonus {
		uint32_t object_id;
		int32_t x, y;
		uint16_t bonus_type;
	};

	struct delete_object {
		uint32_t object_id;
	};

	struct tank_weapon {
		uint8_t level;
		uint16_t cooldown;
		uint16_t max_cooldown;
	};

	uint16_t x, y;
	int32_t left;
	uint16_t width;
	uint32_t xp;
	uint8_t can_upgrade;
	std::vector<tank_weapon> weapons;
	uint8_t weapon;
	std::vector<ground_section> ground_sections;
	std::vector<insert_update_tank> insert_tanks;
	std::vector<insert_update_tank> update_tanks;
	std::vector<delete_object> delete_tanks;
	std::vector<insert_update_bullet> insert_bullets;
	std::vector<insert_update_bullet> update_bullets;
	std::vector<delete_object> delete_bullets;
	std::vector<insert_update_explosion> insert_explosions;
	std::vector<insert_update_explosion> update_explosions;
	std::vector<delete_object> delete_explosions;
	std::vector<insert_update_bonus> insert_bonuses;
	std::vector<insert_update_bonus> update_bonuses;
	std::vector<delete_object> delete_bonuses;
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_view& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::insert_update_tank& m) {
	s.read(m.object_id);
	s.read(m.color);
	s.read(m.x);
	s.read(m.y);
	s.read(m.direction);
	s.read(m.power);
	s.read(m.centihealth);
	s.read(m.name);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::insert_update_bullet& m) {
	s.read(m.object_id);
	s.read(m.x);
	s.read(m.y);
	s.read(m.velocity_x);
	s.read(m.velocity_y);
	s.read(m.has_gravity);
	s.read(m.is_digger);
	s.read(m.start_radius);
	s.read(m.end_radius);
	s.read(m.target_frames);
	s.read(m.frame);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::insert_update_explosion& m) {
	s.read(m.object_id);
	s.read(m.x);
	s.read(m.y);
	s.read(m.target_frames);
	s.read(m.target_radius);
	s.read(m.frame);
	s.read(m.explode_delay);
	s.read(m.is_dirt);
}


template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::insert_update_bonus& m) {
	s.read(m.object_id);
	s.read(m.x);
	s.read(m.y);
	s.read(m.bonus_type);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::delete_object& m) {
	s.read(m.object_id);
}


template<>
inline void data_serialize(data_serializer& s, const scorch_message_view::tank_weapon& m) {
	s.write(m.level);
	s.write(m.cooldown);
	s.write(m.max_cooldown);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view::tank_weapon& m) {
	s.read(m.level);
	s.read(m.cooldown);
	s.read(m.max_cooldown);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_view& m) {
	s.read(m.x);
	s.read(m.y);
	s.read(m.left);
	s.read(m.width);
	s.read(m.xp);
	s.read(m.can_upgrade);

	m.weapons.clear();
	data_deserialize(s, m.weapons);
	/*
	uint8_t len;
	s.read(len);
	m.weapon_counts.clear();
	if (len > 0) {
		s.read(m.weapon);
		for (int i = 0; i < len; i++) {
			uint8_t count;
			s.read(count);
			m.weapon_counts.push_back(count);
		}
	}*/

	uint16_t section_count;
	s.read(section_count);
	for (int i = 0; i < section_count; i++) {
		scorch_message_view::ground_section section;
		s.read(section.left);
		s.read(section.right);

		for (int k = section.left; k < section.right; k++) {
			uint16_t segment_count;
			s.read(segment_count);
			std::vector<scorch_message_view::ground_segment> segments;
			for (int j = 0; j < segment_count; j++) {
				scorch_message_view::ground_segment segment;
				s.read(segment.y1);
				s.read(segment.y2);
				s.read(segment.falling);
				s.read(segment.fall_delay);
				s.read(segment.velocity);
				segments.push_back(segment);
			}
			section.segments.push_back(segments);
		}
		m.ground_sections.push_back(section);
	}

	uint16_t diff_count;
	s.read(diff_count);

	scorch_message_view::insert_update_tank ins_upd_tank;
	scorch_message_view::insert_update_bullet ins_upd_bullet;
	scorch_message_view::insert_update_explosion ins_upd_explosion;
	scorch_message_view::insert_update_bonus ins_upd_bonus;
	scorch_message_view::delete_object del;

	for (int i = 0; i < diff_count; i++) {
		uint8_t state_type;
		uint8_t object_type;
		s.read(state_type);
		s.read(object_type);

		switch (object_type) {
		// TANK
		case nibbles_protocol_object_player:
			switch (state_type) {
			case diff_modify_type_add:
				data_deserialize(s, ins_upd_tank);
				m.insert_tanks.push_back(ins_upd_tank);
				break;
			case diff_modify_type_update:
				data_deserialize(s, ins_upd_tank);
				m.update_tanks.push_back(ins_upd_tank);
				break;
			case diff_modify_type_remove:
				data_deserialize(s, del);
				m.delete_tanks.push_back(del);
				break;
			default:
				assert(false);
				break;
			}
			break;

		// BULLET
		case nibbles_protocol_object_bullet:
			switch (state_type) {
			case diff_modify_type_add:
				data_deserialize(s, ins_upd_bullet);
				m.insert_bullets.push_back(ins_upd_bullet);
				break;
			case diff_modify_type_update:
				data_deserialize(s, ins_upd_bullet);
				m.update_bullets.push_back(ins_upd_bullet);
				break;
			case diff_modify_type_remove:
				data_deserialize(s, del);
				m.delete_bullets.push_back(del);
				break;
			default:
				assert(false);
				break;
			}
			break;

		// EXPLOSION
		case nibbles_protocol_object_explosion:
			switch (state_type) {
			case diff_modify_type_add:
				data_deserialize(s, ins_upd_explosion);
				m.insert_explosions.push_back(ins_upd_explosion);
				break;
			case diff_modify_type_update:
				data_deserialize(s, ins_upd_explosion);
				m.update_explosions.push_back(ins_upd_explosion);
				break;
			case diff_modify_type_remove:
				data_deserialize(s, del);
				m.delete_explosions.push_back(del);
				break;
			default:
				assert(false);
				break;
			}
			break;

		// BONUS
		case nibbles_protocol_object_bonus:
			switch (state_type) {
			case diff_modify_type_add:
				data_deserialize(s, ins_upd_bonus);
				m.insert_bonuses.push_back(ins_upd_bonus);
				break;
			case diff_modify_type_update:
				data_deserialize(s, ins_upd_bonus);
				m.update_bonuses.push_back(ins_upd_bonus);
				break;
			case diff_modify_type_remove:
				data_deserialize(s, del);
				m.delete_bonuses.push_back(del);
				break;
			default:
				assert(false);
				break;
			}
			break;
		default:
			assert(false);
			break;
		}
	}
}


struct scorch_message_join : data_message<scorch_protocol_join> {
	std::string nick;
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_join& m) {
	s.write(m.nick);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_join& m) {
	s.read(m.nick);
}

struct scorch_message_move : data_message<scorch_protocol_move> {
	float direction;
	uint16_t power;
	uint8_t press_space;
	uint8_t press_left;
	uint8_t press_right;
	uint8_t weapon;
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_move& m) {
	s.write(m.direction);
	s.write(m.power);
	s.write(m.press_space);
	s.write(m.press_left);
	s.write(m.press_right);
	s.write(m.weapon);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_move& m) {
	s.read(m.direction);
	s.read(m.power);
	s.read(m.press_space);
	s.read(m.press_left);
	s.read(m.press_right);
	s.read(m.weapon);
}


struct scorch_message_redraw : data_message<scorch_protocol_redraw> {
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_redraw& m) {}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_redraw& m) {}

struct scorch_message_level_up : data_message<scorch_protocol_level_up> {
	uint8_t weapon;
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_level_up& m) {
	s.write(m.weapon);
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_level_up& m) {
	s.read(m.weapon);
}

struct scorch_message_pause : data_message<scorch_protocol_pause> {
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_pause& m) {
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_pause& m) {
}

struct scorch_message_resume : data_message<scorch_protocol_resume> {
};

template<>
inline void data_serialize(data_serializer& s, const scorch_message_resume& m) {
}

template<>
inline void data_deserialize(data_deserializer& s, scorch_message_resume& m) {
}
