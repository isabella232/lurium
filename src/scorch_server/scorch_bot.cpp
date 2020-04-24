#define _USE_MATH_DEFINES
#include <vector>
#include <deque>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "lurium/math/rand2.h"
#include "lurium/math/random.h"
#include "ground.h"
#include "scorch.h"
#include "scorch_bot.h"

scorch_bot::scorch_bot() {
	tank = NULL;
	state = scorch_bot_state::idle;
	counter = idle_time;
}

// update = before tick, spawn players, set direction
// postupdate = after tick, catch death etc
void scorch_bot::post_update(scorch_engine& game) {
	if (tank == NULL) {
		return;
	}

	if (!tank->active) {
		tank = NULL;
		state = scorch_bot_state::idle;
		counter = idle_time;
	}
}


bool trajectory_angle(float v, float x, float y, float g, float* angle1, float* angle2) {
	//http://gamedev.stackexchange.com/questions/53552/how-can-i-find-a-projectiles-launch-angle
	// https://en.wikipedia.org/wiki/Trajectory_of_a_projectile
	auto v2 = v * v;
	auto v4 = v2 * v2;
	auto temp = v4 - g * (g * x * x + 2 * y * v2);
	if (temp < 0) {
		return false;
	}

	auto delta = std::sqrt(temp);
	assert(!isnan(delta));

	*angle1 = atan2((v2 + delta), (g * x));
	*angle2 = atan2((v2 - delta), (g * x));
	//*angle1 = atan((v2 + delta) / (g * x));
	//*angle2 = atan((v2 - delta) / (g * x));
	return true;
}

bool get_launch_parameters(const point& src, const point& dest, float gravity_y, float* result_angle, float* result_power) {

	auto power = (float)rand2(7) + 5;
	auto x = dest.x - src.x;
	auto y = dest.y - src.y;
	float angle1, angle2;
	if (trajectory_angle(power, x, y, gravity_y, &angle1, &angle2)) {
		if (rand2(2) == 0) {
			*result_angle = angle1;
		} else {
			*result_angle = angle2;
		}
		*result_power = power;
		return true;
	}
	return false;
}

int classify_object(scorch_engine& game, scorch_player* tank, scorch_object* object, float& distance, point& position) {
	// prioritering:
	// 0. nære spillere litt under eller over bakken
	// 1. nære bonus litt under over bakken
	// 2. spillere over bakken
	// 3. bonus over bakken
	// 4. spillere under bakken
	// 5. bonus under bakken

	int quarter_view = 640 / 4;
	int near_bury_depth = game.TANK_RADIUS / 2;

	if (object->type == scorch_object_type_tank) {
		if (object == tank) {
			return -1;
		}

		scorch_player* other = (scorch_player*)object;
		auto ground_y = game.terrain.get_height_at(other->position.x);
		distance = other->position.distance(tank->position);
		position = other->position;

		if (other->position.y >= ground_y - near_bury_depth) {
			if (distance < quarter_view) {
				return 0;
			} else {
				return 2;
			}
		} else {
			return 4;
		}

	} else if (object->type == scorch_object_type_bonus) {
		scorch_bonus* other = (scorch_bonus*)object;
		auto ground_y = game.terrain.get_height_at(other->position.x);
		distance = other->position.distance(tank->position);
		position = other->position;

		if (other->position.y >= ground_y - near_bury_depth) {
			if (distance < quarter_view) {
				return 1;
			} else {
				return 3;
			}
		} else {
			return 5;
		}

	} else {
		return -1;
	}
}

void scorch_bot::update(scorch_engine& game) {
	if (tank == NULL) {
		return;
	}

	/*
	if (counter > 0) {
		counter--;
		return;
	}

	if (counter == 0) {
		if (state == scorch_bot_state::idle) {
			// set aim_target
			counter = 50;
			state = scorch_bot_state::aiming;
		}
	}*/

/*	if (tank->cooldown > 0) {
		tank->press_space = false;
		return;
	}*/


	if (state == scorch_bot_state::idle) {
		// find target, set state to aiming, 
		tank->press_space = false;
		if (counter > 0) {
			counter--;
		} else {
			if (select_target(game)) {
				state = scorch_bot_state::aiming;
			}
		}
	}

	if (state == scorch_bot_state::aiming) {
		// move tank->direction towards launch_angle
		if (tank->direction < launch_angle) {
			auto diff = launch_angle - tank->direction;
			tank->direction += std::min( 2 * (float)M_PI / 180.0f, diff); // move by two degree per frame
		} else {
			auto diff = tank->direction - launch_angle;
			tank->direction -= std::min( 2 * (float)M_PI / 180.0f, diff); // move by two degree per frame
		}

		if (tank->direction == launch_angle) {
			tank->power = launch_power / game.max_power;
			tank->press_space = true;
			tank->weapon = launch_weapon;
			state = scorch_bot_state::idle;
			counter = idle_time;
		}
	}

	/*
	if (target_distance < game.width) {

		float angle, power;
		if (get_launch_parameters(tank->position, target_position, -game.gravity.y, &angle, &power)) {

			tank->power = power / game.max_power;
			tank->direction = angle;
			if (target_type == scorch_object_type_bonus) {
				tank->weapon = weapon_type::baby_missile;
			} else {
				// choose a random weapon against players
				//tank->weapon = (weapon_type)rand2(weapon_type::max_weapons);
				// never select baby missile directly, it will be used if there are none left of the random selection
				tank->weapon = (weapon_type)(rand2(weapon_type::max_weapons - 1) + 1);
			}
			tank->press_space = true;
		}
	}*/

	// find players nearby
	// find bonus nearby

	// has obstacle between ? how much up?
	// expect to dig away?

	// when to move? incoming missile, no players/bonus nearby

	// am buried?
}


bool scorch_bot::select_target(scorch_engine& game) {

	int radius = 300;

	rect<int> view(tank->position.x - radius, tank->position.y - radius, tank->position.x + radius, tank->position.y + radius);
	view.constrict_bound(0, 0, game.width, game.height);
	std::vector<scorch_object*> objects;
	game.collider.query(view, objects);

	//tank->press_space = false;

	point target_position(0, 0);
	float target_distance = std::numeric_limits<float>::max();
	scorch_object_type target_type;
	int target_category = -1;

	for (auto o : objects) {
		float distance;
		point position;
		int category = classify_object(game, tank, o, distance, position);

		if (category != -1 && (target_category == -1 || category < target_category || (category == target_category && distance < target_distance))) {
			target_category = category;
			target_distance = distance;
			target_position = position;
			target_type = o->type;
		}
	}

	if (target_distance < game.width) {

		float angle, power;
		if (get_launch_parameters(tank->position, target_position, -game.gravity.y, &launch_angle, &launch_power)) {

			if (target_type == scorch_object_type_bonus) {
				launch_weapon = weapon_type::baby_missile;
			} else {
				// choose a random weapon against players
					launch_weapon = (weapon_type)(rand2(weapon_type::max_weapons) );
			}

			return true;
		}

	}
	return false;

}