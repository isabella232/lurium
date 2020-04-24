#pragma once

#include "lurium/host/game_engine.h"

enum weapon_type {
	baby_missile,
	missile,
	mirv,
	funkybomb,
	dirtbomb,
	digger,
	max_weapons
};

// define weapons/explosions from data, calculate concrete definition from players weapon level
struct weapon_definition {
	std::string name;
	bool is_dirt;
	bool has_gravity;
	bool is_digger;
	bool is_shield;
	
	int start_radius;
	int end_radius;
	int duration;

	int explode_radius;
	float explode_damage;
	int cooldown;

	int mirv_count;
	int bullet_count; // generates all bullets at firing time

	float radius_level_scale;
	float explode_level_scale;
	float damage_level_scale;
	float cooldown_level_scale; // higher level increases cooldown!
	float mirv_level_scale;
	float bullet_level_scale;
};

const std::string weapon_type_name[max_weapons] = {
	"Baby Missile",
	"Missile",
	"M.I.R.V",
	"Funky Bomb",
	"Dirt Bomb",
	"Digger"
};

enum scorch_object_type {
	scorch_object_type_tank,
	scorch_object_type_bullet,
	scorch_object_type_explosion,
	scorch_object_type_bonus,

	scorch_object_type_motion_move = 255
};

typedef game_object<scorch_object_type> scorch_object;
typedef game_motion_object<int, scorch_object_type> scorch_motion_object;

struct scorch_bullet : scorch_motion_object {
	struct scorch_player* player;
	weapon_type weapon;
	pointF velocity;
	pointF position;
	//rect<int> bounds;
	int start_radius, end_radius;
	int frame;
	int target_frames;
	bool has_gravity;
	bool is_digger;
	bool is_dirt;
	int explode_radius;
	float damage;
	int mirv_count;
	bool mirv_released;
};

struct scorch_explosion : scorch_motion_object {
	point position;
	int frame;
	int target_radius;
	int target_frames;
	int explode_delay;
	struct scorch_player* player;
	bool is_dirt;
	float damage;
};

enum scorch_bonus_type {
	health_up,
	health_max,
	shield_max,
	reduce_cooldown,
	weapon
};

struct scorch_bonus : scorch_motion_object {
	point position;
	scorch_bonus_type bonus_type;
};

enum scorch_message_type {
	killed_tank,
	killed_by,
	bonus
};

struct scorch_message {
	scorch_message_type type;
	std::string name;
};

struct weapon_info {
	int cooldown;
	int max_cooldown;
	int level;
	// from level we calculate explosion sizes, cooldown period, warheads/sub-missiles
};

struct scorch_player : scorch_motion_object {
	std::string name;
	int color;
	point position;
	int shield;
	bool press_space;
	bool press_left;
	bool press_right;
	weapon_type weapon;
	std::vector<weapon_info> weapons;
	int xp; // from xp we get level, and available upgrade points
	int upgrades; // spent upgrade points
	bool dirty_weapon;
	float power;
	float direction;
	int max_health;
	int health;
	std::vector<scorch_message> messages;
	int bullet_ref_count;
	bool dead;
};

//bool diffing_quadtree_object_has_changed(scorch_object* object);

struct scorch_engine : game_engine<int, scorch_object_type, scorch_object_type_motion_move> {
	int width;
	int height;
	int frame_rate;
	pointF gravity;
	int TANK_RADIUS;
	int BARREL_RADIUS;
	int BULLET_RADIUS;
	float COOLDOWN_RATE;
	float max_power;

	int timestamp;
	int active_players;
	int active_bonuses;
	int object_counter;
	std::vector<scorch_player> tanks;
	std::vector<scorch_bullet> bullets;
	std::vector<scorch_explosion> explosions;
	terrain_ground terrain;
	std::vector<scorch_bonus> bonuses;
	ground_generator ground_gen;
	int ground_wave_counter;
	std::vector<weapon_definition> weapon_definitions;


	scorch_engine(int arena_width, int arena_height, int frame_rate, int max_players, int max_player_bullets, int max_explosions, int max_bonuses, int seed, int max_terrain_height, int max_terrain_segment, float gravity, float cooldown_rate, float max_fire_power);

	bool is_near_tank(const point& pos, int radius);
	int make_object_id() { object_counter++; return object_counter - 1; }
	rect<int> get_bounds(scorch_player& player);
	void player_fire(scorch_player& player, weapon_type  weapon, float direction, float power);
	std::vector<scorch_bullet>::iterator insert_bullet(scorch_player& player, weapon_type weapon, const pointF& position, const pointF& velocity, bool is_digger, bool is_dirt, bool has_gravity, int start_radius, int end_radius, int duration, int explode_radius, float damage, int mirv_count);
	void explode(scorch_player& player, const point& p, weapon_type weapon, bool is_dirt, int radius, float damage);
	std::vector<scorch_explosion>::iterator insert_explosion(scorch_player& player, const point& p, weapon_type weapon, bool is_dirt, int radius, float damage, int delay);
	void explode_objects(scorch_explosion& explosion);
	void explode_bonus(scorch_explosion& explosion, scorch_bonus& bonus);
	void explode_player(scorch_explosion& explosion, scorch_player& tank);
	bool intersect_line_object(const pointF p1, const pointF p2, pointF& result);
	bool intersect_line_ground(const pointF& p1, const pointF& p2, pointF& result);
	std::vector<scorch_player>::iterator spawn_player(const std::string& name);
	void spawn_bonus();
	void tick();
	void tick_done();
	void process_wave();
	void process_tank(scorch_player& player);
	point get_move_vector(scorch_player& tank, int input_x);
	//void process_ground(scorch_ground& ground);
	bool process_explosion(scorch_explosion& explosion);
	bool process_bullet(scorch_bullet& bullet);
	void process_mirv(scorch_bullet& bullet);

	void remove_player(scorch_player& player);
	void get_tank_coverage(const point& position, int& cover_left, int& cover_right, int& total_left, int& total_right);
	int get_tank_level(const scorch_player& player);
};
