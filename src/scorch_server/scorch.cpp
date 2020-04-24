#define _USE_MATH_DEFINES
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "lurium/math/random.h"
#include "ground.h"
#include "scorch.h"
#include "lurium/math/intersect_ray_sphere.h"
#include "lurium/math/intersect_line_line.h"
#include "lurium/math/area_intersect_sphere_sphere.h"
#include "lurium/math/rand2.h"
#include "lurium/math/pow2.h"
#include "lurium/math/trajectory.h"

const int BONUS_RADIUS = 6;
const int TANK_SPACING = 50;

template<typename T>
T find_inactive(T begin, T end, int timestamp) {
	for (T i = begin; i != end; ++i) {
		if (!i->active /*&& i->deactivate_timestamp != timestamp*/) return i;
	}
	return end;
}


bool diffing_quadtree_object_has_changed(scorch_object* object) {
	if (object->type == scorch_object_type_tank) {
		return true;
	} else if (object->type == scorch_object_type_bullet) {
		return false;
	} else if (object->type == scorch_object_type_explosion) {
		return false;
	} else if (object->type == scorch_object_type_bonus) {
		return false;
	}
	assert(false);
	return false;
}

const std::vector<weapon_definition> scorch_weapon_definitions = {
	{
		"Baby Missile",
		false, true, false, false, // dirt, gravity, digger, shield
		3, 3, // bullet start, end radius
		20, // duration time, used with dirt
		10, // explode radius
		0.33f, // damage
		3000, // cooldown time
		0, 1, // mirv, bullet count

		0.2f, // radius level scale
		1.0f, // explode level scale
		0.5f, // damage level scale
		100.0f, // cooldown scale
		0.0f, // mirvs
		1.0f // bullets
	},
	{
		"Missile",
		false, true, false, false, // dirt, gravity, digger, shield
		3, 3, // bullet start, end radius
		20, // duration time, used with dirt
		20, // explode radius
		1.0f, // damage
		6000, // cooldown time
		0, 1, // mirv, bullet count

		0.2f, // radius level scale
		2.0f, // explode level scale
		0.5f, // damage level scale
		100.0f, // cooldown scale
		0.0f, // mirvs
		0.0f // bullets
	},

	{
		"M.I.R.V",
		false, true, false, false, // dirt, gravity, digger, shield
		3, 3, // bullet start, end radius
		20, // duration time, used with dirt
		10, // explode radius
		0.5f, // damage
		10000, // cooldown time
		2, 1, // mirv, bullet count

		0.2f, // radius level scale
		1.0f, // explode level scale
		0.5f, // damage level scale
		0.2f, // cooldown scale
		0.334f, // mirvs
		0.0f // bullets
	},

	{
		"Funky Bomb",
		false, true, false, false, // dirt, gravity, digger, shield
		4, 4, // bullet start, end radius
		20, // duration time, used with dirt
		10, // explode radius
		0.8f, // damage
		10000, // cooldown time
		0, 1, // mirv, bullet count

		0.2f, // radius level scale
		1.0f, // explode level scale
		0.5f, // damage level scale
		0.2f, // cooldown scale
		0.0f, // mirvs
		0.0f // bullets
	},

	{
		"Dirt Bomb",
		true, true, false, false, // dirt, gravity, digger, shield
		10, 10, // bullet start, end radius
		20, // duration time, used with dirt
		20, // explode radius
		1.0f, // damage
		5000, // cooldown time
		0, 1, // mirv, bullet count

		0.2f, // radius level scale
		1.0f, // explode level scale
		0.5f, // damage level scale
		0.2f, // cooldown scale
		0.0f, // mirvs
		0.0f // bullets
	},
	{
		"Digger",
		false, true, true, false, // dirt, gravity, digger, shield
		5, 10, // bullet start, end radius
		20, // duration time, used with dirt
		10, // explode radius, not used with dirt?
		1.0f, // damage
		3000, // cooldown time
		0, 1, // mirv, bullet count

		0.5f, // radius level scale
		0.0f, // explode level scale
		0.5f, // damage level scale
		0.2f, // cooldown scale
		0.0f, // mirvs
		0.0f // bullets
	},

};


scorch_engine::scorch_engine(int arena_width, int arena_height, int _frame_rate, int max_players, int max_player_bullets, int max_explosions, int max_bonuses, int seed, int max_terrain_height, int max_terrain_segment, float gravity_y, float cooldown_rate, float max_fire_power) 
	: ground_gen(seed, max_terrain_height, max_terrain_segment, arena_width)
	, weapon_definitions(scorch_weapon_definitions)
{
	width = arena_width;
	height = arena_height;
	frame_rate = _frame_rate;
	active_players = 0;
	active_bonuses = 0;
	object_counter = 0;
	timestamp = 1;
	gravity = pointF(0, gravity_y);
	TANK_RADIUS = 8;
	BARREL_RADIUS = 12;
	BULLET_RADIUS = 3;// 10;
	COOLDOWN_RATE = cooldown_rate;
	max_power = max_fire_power;

	tanks.resize(max_players);
	bullets.resize(max_players * max_player_bullets);
	explosions.resize(max_explosions);
	terrain.ground.resize(width);
	bonuses.resize(max_bonuses);

	int collider_size = make_pow2(std::max(width, height));
	int depth = get_pow2(collider_size) - get_pow2(16); // want quad of 16x16 at max depth
	assert(depth >= 0);
	collider.reset(collider_size, collider_size, depth);

	ground_wave_counter = 0;
}

bool scorch_engine::is_near_tank(const point& pos, int radius) {
	
	auto view = rect<int>::from_center(pos.x, pos.y, radius);
	std::vector<scorch_object*> objects;
	collider.query(view, objects);

	for (auto& o : objects) {
		if (o->type == scorch_object_type_tank) {
			return true;
		}
	}

	return false;
}

rect<int> scorch_engine::get_bounds(scorch_player& player) {
	return rect<int>(
		std::max(player.position.x - TANK_RADIUS, 0),
		std::max(player.position.y - TANK_RADIUS, 0),
		std::min(player.position.x + TANK_RADIUS, width),
		std::min(player.position.y + TANK_RADIUS, height));
}

std::vector<scorch_player>::iterator scorch_engine::spawn_player(const std::string& name) {
	// find a location at least some distance from all other players
	auto player = find_inactive(tanks.begin(), tanks.end(), timestamp);
	if (player == tanks.end()) {
		return player;
	}

	player->type = scorch_object_type_tank;
	int attempts = 0;
	do {
		// spawn somewhat away from the edges
		player->position.x = rand2(width - TANK_SPACING - TANK_SPACING) + TANK_SPACING;
		player->position.y = terrain.get_height_at(player->position.x);
		attempts++;
		if (attempts > 1000) {
			return tanks.end();
		}
	} while (is_near_tank(player->position, TANK_SPACING));

	player->name = name;
	player->color = rand2(6);
	player->active = true;
	player->dead = false;
	player->object_id = make_object_id();
	player->direction = M_PI / 2;
	player->press_space = false;
	player->press_left = false;
	player->press_right = false;
	player->power = 0.5;
	player->xp = 0;
	player->upgrades = 0;
	player->max_health = 100;
	player->health = 100;
	player->shield = 0;

	player->weapon = weapon_type::baby_missile;
	player->dirty_weapon = true;

	player->weapons.resize(weapon_definitions.size());
	for (auto& weapon : player->weapons) {
		weapon.cooldown = 0;
		weapon.max_cooldown = 1; // hm, from wepdef
		weapon.level = 0;

	}

	player->bounds = get_bounds(*player);

	spawn(*player);
	active_players++;
	return player;
}

void scorch_engine::spawn_bonus() {
	auto bonus = find_inactive(bonuses.begin(), bonuses.end(), timestamp);

	if (bonus == bonuses.end()) {
		return ;
	}

	bonus->type = scorch_object_type_bonus;
	bonus->bonus_type = (scorch_bonus_type)rand2(5);
	bonus->active = true;
	//bonus->deactivate_timestamp = -1;
	bonus->object_id = make_object_id();

	// TODO: dont inside terrain or player
	bonus->position.x = rand2(width);
	bonus->position.y = rand2(200);
	
	bonus->bounds = rect<int>::from_center(bonus->position.x, bonus->position.y, BONUS_RADIUS);
	bonus->bounds.constrict_bound(0, 0, width, height);

	//collider.insert(bonus->bounds, &*bonus);
	spawn(*bonus);
	active_bonuses++;
}

void scorch_engine::tick() {

	//std::cout << "tick" << std::endl;
	if (active_bonuses < bonuses.size()) {
		spawn_bonus();
	}

	process_wave();

	// process bullets before players; this removes players, inserts explosions, modifies terrain. also avoids processing already-inserted bullets

	// TODO: process explosion already? what if bullet explodes on first?

	for (int i = 0; i < bullets.size(); i++) {
		auto& bullet = bullets[i];
		if (!bullet.active || bullet.is_despawning) {
			continue;
		}
		if (process_bullet(bullet)) {
			//bullet.active = false;
			//bullet.deactivate_timestamp = timestamp;
			bullet.player->bullet_ref_count--;
			despawn(bullet);
			//collider.remove(bullet.bounds, &bullet);
		} else {
			// the collider doest know about moving bullets, it keys bullets by the bbox of the trajectory
		}
	}

	for (auto& player : tanks) {
		if (!player.active || player.is_despawning || player.is_spawning) {
			continue;
		}
		if (player.dead) {
			if (player.bullet_ref_count == 0) {
				//player.active = false;
				// TODO: this is where we despawn the playe??
			}
			continue;
		}
		process_tank(player);
	}

	terrain.process(gravity.y);
	//for (auto& g : terrain.ground) {
		//process_ground(g);
	//}

	for (int i = 0; i < explosions.size(); i++) {
		auto& explosion = explosions[i];
		if (!explosion.active || explosion.is_despawning) {
			continue;
		}
		if (process_explosion(explosion)) {
			//explosion.active = false;
			//explosion.deactivate_timestamp = timestamp;
			despawn(explosion);
			//collider.remove(explosion.bounds, &explosion);
		} else {
			// ..
		}
	}

}

void scorch_engine::tick_done() {

	// NOTE: vikings server does this first thing in tick() .. has bug, but not as prevalent. also; are there other problems? vikings was originally send current state + input to advance to next frame
	// scorch does not move upon spawn, and commits spawns after transmitting state to client
	process_spawns();

	for (auto& player : tanks) {
		player.dirty_weapon = false;
		player.messages.clear();
	}

	//collider.apply();
	timestamp++;

	ground_wave_counter++;
	if (ground_wave_counter >= ground_gen.max_results) {

		// calculate, by estimating how much a player destroys on average, by how much total there is

		ground_wave_counter = -width * 5; // wait so long until restarting next wave, at 10k width * 5 gives reset ca 30 mins
		ground_gen.reset();
	}

}

void scorch_engine::process_wave() {
	// 1 insert/move terrain here
	// 2 shift players too, remain in same amount of shit, or on top
	
	// query players at (exactly?) wave position

	if (ground_wave_counter >= 0) {
		
		int y = ground_gen.next();

		int old_y = terrain.get_height_at(ground_wave_counter);

		terrain.grow_terrain(ground_wave_counter, 0, y);

		rect<int> gg(ground_wave_counter, 0, ground_wave_counter, height);
		std::vector<scorch_object*> wave_objects;
		collider.query(gg, wave_objects);

		for (auto& wave_object : wave_objects) {
			if (wave_object->is_despawning) {
				continue;
			}

			if (wave_object->type == scorch_object_type::scorch_object_type_tank) {
				scorch_player* wave_tank = (scorch_player*)wave_object;
				if (wave_tank->position.y <= old_y && y > old_y && wave_tank->position.x == ground_wave_counter) {

					//collider.remove(wave_tank->bounds, wave_object);
					wave_tank->position.y += (y - old_y);

					//tank.position.x = next_x;
					//tank.position.y = next_y;

					//update_bounds(*wave_tank);
					//spawn(*wave_tank);
					//collider.insert(wave_tank->bounds, wave_object);
				}
			}
		}
	}

	// incremented and looped in tick_done, so we transmit where its actually at
}


void scorch_engine::player_fire(scorch_player& player, weapon_type weapon, float direction, float power) {

	if (player.weapons[weapon].cooldown > 0) {
		return;
	}

	
	const weapon_definition& wepdef = weapon_definitions[weapon];
	const weapon_info& weplev = player.weapons[weapon];

	// TODO: some kind of exponential level benefit?
	int start_radius = wepdef.start_radius + wepdef.radius_level_scale * weplev.level;
	int end_radius = wepdef.end_radius + wepdef.radius_level_scale * weplev.level;
	int explode_radius = wepdef.explode_radius + wepdef.explode_level_scale * weplev.level;
	float damage = wepdef.explode_damage + wepdef.damage_level_scale * weplev.level;
	int mirv_count = wepdef.mirv_count + (int)(wepdef.mirv_level_scale * weplev.level);
	// TODO: cooldown should depend on framerate; change framerate = same cooldown; evt, vi har ratioen fra config..? som er calculert fra framerate
	int cooldown = (wepdef.cooldown + wepdef.cooldown_level_scale * weplev.level) / 1000.0f * frame_rate;

	int bullet_count = wepdef.bullet_count + wepdef.bullet_level_scale * weplev.level;

	pointF direction_vector(cos(direction), sin(direction));
	pointF position = pointF(player.position.x, player.position.y).add(direction_vector.multiply_scalar(BARREL_RADIUS));

	// if shield, boost shield

	// if digger, insert digger type bullet
	assert(bullet_count > 0);
	for (int i = 0; i < bullet_count; ++i) {
		// change velocity  by some degrees
		pointF velocity = direction_vector.multiply_scalar(power * max_power + i / 10.0f);
		insert_bullet(player, weapon, position, velocity, wepdef.is_digger, wepdef.is_dirt, wepdef.has_gravity, start_radius, end_radius, wepdef.duration, explode_radius, damage, mirv_count);
	}

/*	if (weapon != weapon_type::baby_missile) {
		player.weapon_counts[weapon]--;
		if (player.weapon_counts[weapon] == 0) {
			player.weapon = weapon_type::baby_missile;
		}
	}*/

	// TOOD: set cooldown in insert_bullet? or move wepdefs in here, and take full parameters on insert_bullet() yey
	player.dirty_weapon = true;
	player.weapons[weapon].cooldown = cooldown;
	player.weapons[weapon].max_cooldown = cooldown;

}

std::vector<scorch_bullet>::iterator scorch_engine::insert_bullet(scorch_player& player, weapon_type weapon, const pointF& position, const pointF& velocity,
	bool is_digger, bool is_dirt, bool has_gravity, int start_radius, int end_radius, int duration, int explode_radius, float damage, int mirv_count) {
	auto bullet = find_inactive(bullets.begin(), bullets.end(), timestamp);
	
	if (bullet == bullets.end()) {
		return bullet;
	}

	//wepdef.mirv_count 
	// bullet gets all the relevant info for explosion etc at firing time? cant upgrade while bullet is in the air and make bigger explo. wouldve been cool though
	// scale def by the players level on this weapon


	bullet->type = scorch_object_type_bullet;
	bullet->mirv_released = false;
	bullet->frame = 0;
	//bullet->target_frames = 0; // TODO: unused for non-diggers ? or calculate from trajectory @ 0, or per-weapon for lasers and diggers?
	bullet->active = true;
	bullet->object_id = make_object_id();
	//bullet->deactivate_timestamp = -1;
	bullet->velocity = velocity;// .multiply_scalar((float)power / 100.0f);
	bullet->position = position;// pointF(player.position.x, player.position.y).add(velocity.multiply_scalar(TANK_RADIUS));
	bullet->weapon = weapon;
	bullet->player = &player;

	bullet->is_digger = is_digger;
	bullet->is_dirt = is_dirt;
	bullet->has_gravity = has_gravity;
	bullet->target_frames = duration; // unused for non-digger
	bullet->start_radius = start_radius;
	bullet->end_radius = end_radius; // TANK_RADIUS * 2 + (10 - std::min(velocity.distance(pointF(0, 0)), 10.0)) * 2;
	bullet->mirv_count = mirv_count;
	bullet->explode_radius = explode_radius;
	bullet->damage = damage;

	/*
	switch (weapon) {
		case digger:
			bullet->is_digger = true;
			bullet->has_gravity = false;
			bullet->target_frames = 20;
			bullet->start_radius = BULLET_RADIUS;
			bullet->end_radius = TANK_RADIUS * 2 + (10 - std::min(velocity.distance(pointF(0, 0)), 10.0) ) * 2;
			break;
		default:
			bullet->is_digger = false;
			bullet->has_gravity = true;
			bullet->target_frames = 0;
			bullet->start_radius = BULLET_RADIUS;
			bullet->end_radius = BULLET_RADIUS;
			break;
	}*/

	float xmax, ymax;
	trajectory::bounding_box(position.y, velocity.x, velocity.y, -gravity.y, &xmax, &ymax);

	bullet->bounds.left = std::floor(std::min(position.x, position.x + xmax)); //std ::max((int)position.x - 320, 0);
	bullet->bounds.top = 0;
	bullet->bounds.right = std::ceil(std::max(position.x, position.x + xmax)); //std::min((int)position.x + 320, width);
	bullet->bounds.bottom = std::ceil(ymax);// height;
	bullet->bounds.constrict_bound(0, 0, width, height);
	// TODO: calculate bounds of trajectory, ie max height, and landing at 0 given starting height etc
	// quickfix: use firing player's view as bbox, and clip bullets to it

	//collider.insert(bullet->bounds, &*bullet);
	spawn(*bullet);
	player.bullet_ref_count++;

	return bullet;// (int)std::distance(bullets.begin(), bullet);
}

void scorch_engine::get_tank_coverage(const point& position, int& cover_left, int& cover_right, int& total_left, int& total_right) {
	// find out how much ground is covering the tank, TODO: on each side
	// skal vi trace ytterpunktene av tanken, eller selve grounden?

	// TODO: nå har jeg en teori om at vi finner terrain/tank terrain intersection area, da kan vi grave oss inn opptil halvveis uansett hvor mye som er on top

	cover_left = 0;
	cover_right = 0;
	total_left = 0;
	total_right = 0;

	for (int i = -TANK_RADIUS; i < TANK_RADIUS;  i++) {
		auto x = position.x + i;
		if (x < 0 || x >= terrain.ground.size()) {
			continue;
		}

		// dette er tanken sin y1, y2
		int tank_height = std::ceil(std::sqrt((double)TANK_RADIUS * TANK_RADIUS - i * i));
		//int tank_y1 = tank.position.y;// - height; regner bare på øvre halvdel av tanken
		int tank_y2 = position.y + tank_height;
		// skal 


		auto segments = terrain.ground[position.x + i].segments;

		// how much ground is above tank.position.y; or find the Y along top of tank border
		for (const auto& segment : segments) {

			if (segment.y2 > position.y && segment.y1 <= tank_y2) {
				auto y1 = std::max((int)segment.y1, position.y);
				auto y2 = std::min((int)segment.y2, tank_y2);
				if (i < 0) {
					cover_left += y2 - y1;
				} else {
					cover_right += y2 - y1;
				}
			}

			if (segment.y2 > position.y) {
				auto y1 = std::max((int)segment.y1, position.y);
				if (i < 0) {
					total_left += (int)segment.y2 - y1;
				} else {
					total_right += (int)segment.y2 - y1;
				}
			}
		}

	}
}

point scorch_engine::get_move_vector(scorch_player& tank, int input_x) {
	assert(input_x >= -1 && input_x <= 1);

	int fall_y = 2;


	// do not move outside
	auto is_in_range = tank.position.x + input_x >= 0 && tank.position.x + input_x < (int)terrain.ground.size();
	if (!is_in_range) {
		return point(0, 0);
	}

	// do not move if falling TODO, does not register if falling and inside dirt and it exploded below, can move in some cases now:
	auto ground_y = terrain.get_height_at(tank.position.x);

	bool is_falling = tank.position.y > ground_y;
	int max_fall = tank.position.y - ground_y;

	if (input_x == 0) {
		if (is_falling) {
			return point(0, -std::min(fall_y, max_fall));
		} else {
			return point(0, 0);
		}
	}

	if (tank.position.y >= ground_y) {
		// outer case 1: exactly on top or falling
		int max_climb = (TANK_RADIUS / 2);
		auto next_ground_y = terrain.get_height_at(tank.position.x + input_x);

		bool is_wall = next_ground_y > ground_y;
		bool is_on_wall = next_ground_y > tank.position.y;
		if (is_wall || is_on_wall) {
			if (is_on_wall) {
				// change right/left to upwards motion
				return point(0, 1);
			} else {
				// climb onto the wall edge
				return point(input_x, tank.position.y - std::max(tank.position.y, next_ground_y));
			}
		} else if (!is_falling) {
			// moving normally on top of the ground, fall or climb
			return point(input_x, tank.position.y - std::max(tank.position.y, next_ground_y));
			//move.y = tank.position.y - std::max(tank.position.y, next_ground_y);
		} else {
			// do not move if falling and not next to a wall
			return point(0, -std::min(fall_y, max_fall));
		}
	} else {
		// outer case 2: below top, inside dirt
		// allow to move if there is below a certain total coverage, and f.ex move upwards

		auto movable_tank_area = (M_PI * TANK_RADIUS * TANK_RADIUS);// / 2.0;

		int cover_left, cover_right, total_left, total_right;
		get_tank_coverage(tank.position, cover_left, cover_right, total_left, total_right);

		if (total_left + total_right < movable_tank_area) {
			// can move upwards if there is below a certain amount of dirt
			/*next_ground_y = ground_y;
			move.x = 0;// tank.position.x;
			move.y = 1;// tank.position.y + 1;
			*/
			return point(0, 1);
		} else {
			// dug too deep down to move anywhere
			return point(0, 0);
		}
	}
	//return move;
}

void scorch_engine::process_tank(scorch_player& tank) {

	// weapon cooldones
	for (auto& weapon : tank.weapons) {
		if (weapon.cooldown > 0) {
			weapon.cooldown--;
		}
	}

	if (tank.press_space) {
		player_fire(tank, tank.weapon, tank.direction, tank.power);
	}

	point move(0, 0);
	int input_x = 0;

	if (tank.press_left || tank.press_right) {
		if (tank.press_left) {
			input_x--;
		}

		if (tank.press_right) {
			input_x++;
		}
	}

	move = get_move_vector(tank, input_x);

	if (move.x != 0 || move.y != 0) {
		//collider.remove(tank.bounds, &tank);
		tank.position.x += move.x;// next_x;
		tank.position.y += move.y;// next_y;
	}

	// last chance to move tank; this includes falling; always update
	auto new_bounds = get_bounds(tank);
	update_motion(tank, new_bounds);

}

void scorch_engine::explode_player(scorch_explosion& explosion, scorch_player& tank) {
	auto tank_area = M_PI * TANK_RADIUS * TANK_RADIUS;
	// NOTE: this assume the tank is a sphere, not a half-sphere
	auto intersect_area = area_intersect_sphere_sphere<float>(tank.position.x, tank.position.y, TANK_RADIUS, explosion.position.x, explosion.position.y, explosion.target_radius);

	auto coverage = intersect_area / tank_area;
	// so we say full coverage is 100, which is enough to kill a newly spawned player
	tank.health -= 100.0f * coverage * explosion.damage;

	if (tank.active && tank.health <= 0) {
		scorch_message killed_message = { scorch_message_type::killed_by, explosion.player->name };
		tank.messages.push_back(killed_message);

		scorch_message kill_message = { scorch_message_type::killed_tank, tank.name };
		explosion.player->messages.push_back(kill_message);
		
		explosion.player->xp += tank.xp;
		
		tank.dead = true;
		//on_remove_player(tank); // incl killing player, if any må gjøre funky lookup tank->client .. urk to ganger
		remove_player(tank);
	}

}

void scorch_engine::explode_bonus(scorch_explosion& explosion, scorch_bonus& bonus) {
	// award bonus to the player that caused the explosion
	// also make sure the spheres are actually overlapping
	// remove bonus from collider
	// server shold transmit "bonus message" to player? diffing system on each player, ie to support the already established updating stuff
	// skal vi inserte noe? meeh.. evt, vi ska inserte en flygende "yay you won" der som bonusen var?
	// eller skal vi ha events, "you killed Y", "X killed Y", "you got health", "you got M.I.R.V"
	// basically metadata for "remove bonus" ?

	auto area = area_intersect_sphere_sphere(explosion.position.x, explosion.position.y, explosion.target_radius, bonus.position.x, bonus.position.y, BONUS_RADIUS);
	if (area == 0) {
		return;
	}

	weapon_type prise;
	//switch (bonus.bonus_type) {
		//case scorch_bonus_type::weapon:
			// random weapon!
			// view sier hvilke våpen vi har, bitmask!
			// alså info om self, dette enabler bit for våpen! må sende til klient!
			// skal det være noe juice på skjermen der - f.eks en melding, melding med posisjon??
			// som diep, vi sender meldinger, som vises på klient i en kort periode, men serveren styrer dette,.. diffen kanskje ligger i client klassen?? men da kan vi ikke kjøre update-greiene,
			// dirty flag? "updated" .. men HVA? og hvi vi skal sende diff på feltnivå, så trenger vi kopi, ellr felt-flag
			prise = (weapon_type)(rand2(weapon_type::max_weapons - 1) + 1);
			// TODO: award more missiles than nukes!
			//explosion.player->weapon_counts[prise] += weapon_type_count[prise];
			//explosion.player->weap
			explosion.player->xp += 10;
			//explosion.player->dirty_weapon = true;
			
			scorch_message bonus_message = { scorch_message_type::bonus, "Yay!" }; // weapon name in engine, or bonus name, like health
			explosion.player->messages.push_back(bonus_message);

			//break;
	//}

	//bonus.active = false;
	//bonus.deactivate_timestamp = timestamp;
	//collider.remove(bonus.bounds, &bonus);
	despawn(bonus);
	active_bonuses--;
}

void scorch_engine::explode_objects(scorch_explosion& explosion) {
	std::vector<scorch_object*> objects;
	collider.query(explosion.bounds, objects);

	for (auto o : objects) {
		if (o->is_despawning) {
			continue;
		}
		//assert(o->active);
		switch (o->type) {
			case scorch_object_type_tank:
				explode_player(explosion, *(scorch_player*)o);
				break;
			case scorch_object_type_bonus:
				explode_bonus(explosion, *(scorch_bonus*)o);
				break;
		}
	}
}

bool scorch_engine::process_explosion(scorch_explosion& explosion) {
	// TODO: kan have andre typer explosion som går av på forskjellig tidspunt, f.ex funky bomb
	// da lager vi flere explosion med forskjellig delay
	if (explosion.explode_delay > 0) {
		explosion.explode_delay--;
		return false;
	}

	if (explosion.frame == explosion.target_frames - 1) {
		// IF !is_dirty, else: insert terrin? 
		terrain.modify_terrain(explosion.position.x, explosion.position.y, explosion.target_radius, explosion.is_dirt);
		if (!explosion.is_dirt) {
			// Query current for players/bonus in explosion
			explode_objects(explosion);
		}
	}
	explosion.frame++;
	return explosion.frame >= explosion.target_frames;
}

bool scorch_engine::intersect_line_ground(const pointF& p1, const pointF& p2, pointF& result) {

	// TODO: doesnt handle straight up/down, (fixed near angles)

	int x1, x2;
	int d;
	pointF c1, c2;

	if (p1.x < p2.x) {
		// bullet moves left to right
		// extend p1 such that p1.x is floor(p1.x)
		// extend p2 such that p2.x is ceil(p2.x)
		x1 = std::floor(p1.x);
		x2 = std::ceil(p2.x);
		d = 1;
	} else if (p1.x > p2.x) {
		// bullet moves right to left
		// extend p1 such that p1.x is ceil(p1.x)
		// extend p2 such that p2.x is floor(p2.x)
		x1 = std::ceil(p1.x);
		x2 = std::floor(p2.x);
		d = -1;
	} else if (p1.x == p2.x) {
		// bullet moves straight down, collide as if moving a px to the right
		x1 = std::floor(p1.x);
		x2 = x1 + 1;
		d = 1;
	}

	c1 = pointF(x1, p1.y);
	c2 = pointF(x2, p2.y);

	// c1->c2 is the bullet trajectory extended to full pixel width

	for (auto x = x1; x != x2; x += d) {
		const auto& ground = terrain.ground[x];

		float ua, ub;
		if (ground.segments.empty()) {
			bool intersects = intersect_line(c1, c2, pointF(x + d * 0.5, 0), pointF(x + d * 0.5, -1), ua, ub);
			if (intersects && ua >= 0.0 && ua <= 1.0 && ub >= 0.0 /*&& ub <= 1.0*/) {
				// TODO: Clip at 0
				result.x = c1.x + ua * (c2.x - c1.x);
				result.y = c1.y + ua * (c2.y - c1.y);
				return true;
			}
		}

		for (auto i = 0; i < ground.segments.size(); i++) {
			const auto& segment = ground.segments[i];

			/*if (x1 == x2) {
				// check if p's y are inside the segment, return the point
			} else*/ {

				bool intersects = intersect_line(c1, c2, pointF(x + d * 0.5, segment.y2), pointF(x + d * 0.5, segment.y1), ua, ub);

				if (segment.y1 <= 0) {
					// Pretend first segment extends to negative infinity
					if (intersects && ua >= 0.0 && ua <= 1.0 && ub >= 0.0 /*&& ub <= 1.0*/) {
						// TODO: Clip at 0
						result.x = c1.x + ua * (c2.x - c1.x);
						result.y = c1.y + ua * (c2.y - c1.y);
						return true;
					}
				} else {

					if (intersects && ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
						result.x = c1.x + ua * (c2.x - c1.x);
						result.y = c1.y + ua * (c2.y - c1.y);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool intersect_tank(const pointF& p1, const pointF& p2, scorch_player& tank, int tank_radius, pointF& result) {
	// ray spere can have two results, need the "right one"
	// also, onyl check intersection on top half circe, and tracks??
	double mu1, mu2;
	bool intersects = intersect_ray_sphere(p1, p2, pointF(tank.position.x, tank.position.y), tank_radius, &mu1, &mu2);

	auto mu = std::min(mu1, mu2);
	if (!intersects || mu < 0 || mu > 1) {
		return false;
	}

	result = p1.add(p2.subtract(p1).multiply_scalar(mu));

	return true;
}

bool intersect_bonus(const pointF& p1, const pointF& p2, scorch_bonus& bonus, pointF& result) {
	double mu1, mu2;
	bool intersects = intersect_ray_sphere(p1, p2, pointF(bonus.position.x, bonus.position.y), BONUS_RADIUS, &mu1, &mu2);

	auto mu = std::min(mu1, mu2);
	if (!intersects || mu < 0 || mu > 1) {
		return false;
	}

	result = p1.add(p2.subtract(p1).multiply_scalar(mu));

	return true;
}

bool scorch_engine::intersect_line_object(const pointF p1, const pointF p2, pointF& result) {

	// Query collider for tanks/bonus near the bullet

	//rect<int> rc(p2.x - BULLET_RADIUS, p2.y - BULLET_RADIUS, p2.x + BULLET_RADIUS, p2.y + BULLET_RADIUS);
	rect<int> rc(p2.x, p2.y, p2.x, p2.y);

	std::vector<scorch_object*> near_objects;
	collider.query(rc, near_objects);

	for (auto& i : near_objects) {
		if (i->is_despawning) {
			continue;
		}
		if (i->type == scorch_object_type_tank) {
			if (intersect_tank(p1, p2, (scorch_player&)*i, TANK_RADIUS, result)) {
				return true;
			}
		} else if (i->type == scorch_object_type_bonus) {
			if (intersect_bonus(p1, p2, (scorch_bonus&)*i, result)) {
				return true;
			}
		}
	}
	return false;
}

bool scorch_engine::process_bullet(scorch_bullet& bullet) {
	auto position = bullet.position.add(bullet.velocity);

	if (bullet.has_gravity) {
		position = position.add(gravity);
	}

	if (position.x < 0 || position.x >= terrain.ground.size()) {
		return true;
	}

	if (bullet.is_digger) {
		// dig out where the bullet is, at some radius - also return true after some time
		// also TODO: dig line with borderwidth for whole velocity
		auto radius_range = bullet.end_radius - bullet.start_radius;
		auto radius_unit = bullet.frame / (float)bullet.target_frames;
		auto bullet_radius = (int)(bullet.start_radius + radius_unit  * radius_range);
		terrain.modify_terrain(position.x, position.y, bullet_radius, false);

		if (bullet.frame >= bullet.target_frames) {
			return true;
		}
	} else {
		pointF p;
		// Hit test players/bonuses or ground:
		bool intersects = intersect_line_object(bullet.position, position, p) || intersect_line_ground(bullet.position, position, p);

		if (intersects) {
			explode(*bullet.player, point((int)p.x, (int)p.y), bullet.weapon, bullet.is_dirt, bullet.explode_radius, bullet.damage);
			return true;
		} 
	}
	
	// if mirv && at max height, change into multiple bullets
	if (bullet.mirv_count > 0) {
		process_mirv(bullet);
	}

	bullet.velocity = position.subtract(bullet.position);
	bullet.position = position;
	bullet.frame++;

	return false;
}

void scorch_engine::process_mirv(scorch_bullet& bullet) {
	// Releases "vehicles" at top point
	if (bullet.velocity.y <= 0 && !bullet.mirv_released) {
		// Create new bullets with shorter&longer velocities
		for (int i = -bullet.mirv_count; i < 0; i++) {

			auto projectile = insert_bullet(*bullet.player, weapon_type::mirv, bullet.position, bullet.velocity.multiply_scalar(1 + i * 0.1f), 
				bullet.is_digger, bullet.is_dirt, bullet.has_gravity, bullet.start_radius, bullet.end_radius, bullet.target_frames, bullet.explode_radius, bullet.damage, 0);
			projectile->mirv_released = true;
		}
		
		bullet.mirv_released = true;
	}

}

void scorch_engine::explode(scorch_player& player, const point& p, weapon_type weapon, bool is_dirt, int radius, float damage) {
	insert_explosion(player, p, weapon, is_dirt, radius, damage, 0);
	
	if (weapon == weapon_type::funkybomb) {
		for (int i = 0; i < 4; i++) {
			int x = rand2(80) - 40;
			int y = rand2(100) - 50;
			int delay = (i + 1) * 15;
			auto explosion = insert_explosion(player, point(p.x + x, p.y + y), weapon, is_dirt, radius, damage, delay);
		}
	}

}

std::vector<scorch_explosion>::iterator scorch_engine::insert_explosion(scorch_player& player, const point& p, weapon_type weapon, bool is_dirt, int radius, float damage, int delay) {
	auto explosion = find_inactive(explosions.begin(), explosions.end(), timestamp);
	if (explosion == explosions.end()) {
		return explosion;
	}

	assert(radius > 0);

	explosion->type = scorch_object_type_explosion;
	explosion->active = true;
	//explosion->deactivate_timestamp = -1;
	explosion->position = p;
	explosion->target_frames = 20;
	explosion->target_radius = radius;
	explosion->object_id = make_object_id();
	explosion->frame = 0;
	explosion->bounds = rect<int>(p.x - radius, 0, p.x + radius, height);
	explosion->bounds.constrict_bound(0, 0, width, height);
	explosion->explode_delay = delay;
	explosion->damage = damage;
	explosion->is_dirt = is_dirt;
	explosion->player = &player;

	// we keep the explsion in the collider for the duration, because it can have mutple explosin at different times
	spawn(*explosion);
	return explosion;
}

void scorch_engine::remove_player(scorch_player& player) {
	despawn(player);
	active_players--;
}

int scorch_engine::get_tank_level(const scorch_player& player) {
	auto constant = 0.3f;
	return (int)(constant * sqrt((float)player.xp));

}