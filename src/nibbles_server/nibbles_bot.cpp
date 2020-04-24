#include <vector>
#include <deque>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "lurium/math/rand2.h"
#include "nibbles.h"
#include "nibbles_bot.h"

int index_from_point(double vx, double vy);
extern point directions[];
bool colliding_player_body(const point& position, const nibbles_player& other_player);

nibbles_bot::nibbles_bot() { 
	client_id = -1;
	seek_position = point(-1, -1);
	last_force = 0;
}

// update = before tick, spawn players, set direction
// postupdate = after tick, catch death etc
void nibbles_bot::post_update(nibbles& game) {
	if (client_id == -1) {
		return;
	}

	if (!game.players[client_id].active) {
		client_id = -1;
		seek_position = point(-1, -1);
		last_force = 0;
	}
}


void nibbles_bot::update(nibbles& game) {
	if (client_id == -1) {
		/*client_id = game.spawn_player(botNames[rand2(botNames.size())]);
		if (client_id == -1) {*/
			return; // full
		//}
	}

	int radius = 20;
	nibbles_player& player = game.players[client_id];
	rect<int> view(player.position.x - radius, player.position.y - radius, player.position.x + radius, player.position.y + radius);
	std::vector<nibbles_base*> objects;
	game.collider.current.query(view, objects);

	// objects is basically the gamestate we have in js
	update_bitmap(20, player.position, objects);

	classify_quadrants(game, player, objects);

	if (seek_position == player.position) {
		// arrived at seek target
		seek_position = point(-1, -1);
	}

	if (seek_position.x != -1) {
		auto relative_position = seek_position.subtract(player.position);
		auto qi = index_from_point(relative_position.x, relative_position.y);
		assert(qi != -1);
		if (quadrants[qi].blocked) {
			// blocked since the point was selected
 			seek_position = point(-1, -1);
		}
	}

	if (seek_position.x == -1) {
		// pick a new target
		select_target(player);
	}

	int move_direction = player.direction_index;
	int opposite_direction = (move_direction + 2) % 4;

	int want_direction = -1;

	if (seek_position.x != -1) {
		auto relative_position = seek_position.subtract(player.position);
		if (
			(move_direction == 0 && relative_position.x > 0) ||
			(move_direction == 1 && relative_position.y > 0) ||
			(move_direction == 2 && relative_position.x < 0) ||
			(move_direction == 3 && relative_position.y < 0))
		{
			want_direction = move_direction;
		} else {
			want_direction = index_from_point(relative_position.x, relative_position.y);
			assert(want_direction != -1);
		}
	} else {
		// there nothing (viable) to eat in sight, just keep moving for now
		want_direction = move_direction;
	}

	if (want_direction != -1 && quadrants[want_direction].obstruct) {
		// pick another direction, the least crowded?
		want_direction = -1;
	}

	if (want_direction == -1) {
		std::vector<int> primaryDirections;
		std::vector<int> secondaryDirections;
		for (int i = 0; i < 4; i++) {
			auto& quadrant = quadrants[i];
			if (quadrant.obstruct) {
				continue;
			}
			if (quadrant.blocked) {
				// blcoked= second choice
				secondaryDirections.push_back(i);
			} else {
				primaryDirections.push_back(i);
			}
		}

		// if theres primary, pick a random, otherwise pick a random secondary
		if (!primaryDirections.empty()) {
			want_direction = primaryDirections[rand2(primaryDirections.size())];
		} else if (!secondaryDirections.empty()) {
			want_direction = secondaryDirections[rand2(secondaryDirections.size())];
		}
	}

	if (want_direction == opposite_direction) {
		// cant go in opposite direction, randomise +1 or +3  - could even include forward as an option, but wth less probablitily
		// or go in the direction with the higest score of the two
		auto dir1 = (want_direction + 1) % 4;
		auto dir2 = (want_direction + 3) % 4;

		double dir_score1 = quadrants[dir1].score;
		double dir_score2 = quadrants[dir2].score;

		if (dir_score1 > dir_score2 && !quadrants[dir1].obstruct) {
			want_direction = dir1;
		} else if (!quadrants[dir2].obstruct) {
			want_direction = dir2;
		} else {
			want_direction = move_direction; // kep moving
		}
	}

	if (want_direction != -1) {
		game.set_input(player, want_direction, false);
	}
}

void nibbles_bot::select_target(nibbles_player& this_player) {

	// TODO: if there is something straight ahead, keep eating
	// or move towards nearest

	auto& move_quadrant = quadrants[this_player.direction_index];
	if (last_force != 1 && move_quadrant.nearest_distance > 0 && move_quadrant.nearest_distance < 2) {
		seek_position = move_quadrant.nearest_target->position;
		// dont force if its 1 since last force
		last_force = 0;
		return;
	}

	last_force++;

	std::vector<int> can_directions;
	double want_score = 0;
	for (int i = 0; i < 4; i++) {

		auto& quadrant = quadrants[i];
		if (quadrant.blocked || quadrant.nearest_target == nullptr) {
			continue;
		}

		auto score = quadrants[i].score;
		if (score > want_score) {
			want_score = score;
			can_directions.clear();
			can_directions.push_back(i);
		} else if (score > 0 && score == want_score) {
			can_directions.push_back(i);
		}
	}

	int want_direction = -1;
	if (!can_directions.empty()) {
		want_direction = can_directions[rand2(can_directions.size())];
	}

	if (want_direction != -1) {
		seek_position = quadrants[want_direction].nearest_target->position;
	} else {
		seek_position = point(-1, -1);
	}
}

void nibbles_bot::update_bitmap(int radius, const point& origin, std::vector<nibbles_base*>& objects) {
	bitmap_size = (radius * 2) + 1;
	bitmap.resize(bitmap_size * bitmap_size);
	bitmap_center = point(radius, radius);
	bitmap_origin = origin.subtract(bitmap_center);

	memset(&bitmap.front(), 0, bitmap.size());

	for (auto& o : objects) {
		if (o->type == nibbles_object_type_player) {
			draw_player((nibbles_player&)*o);
		}
	}
}


void nibbles_bot::draw_player(nibbles_player& player) {

	assert(player.segments_position == player.position);
	auto p1 = player.segments_position;
	p1 = p1.subtract(bitmap_origin);
	for (auto& segment : player.segments) {

		for (int i = 0; i < segment.length; ++i) {
			draw_pixel(p1, 1);
			p1 = p1.add(directions[segment.direction]);
		}

	}
}

void nibbles_bot::draw_pixel(const point& p, char c) {
	if (p.x < 0 || p.y < 0 || p.x >= bitmap_size || p.y >= bitmap_size) {
		return;
	}

	bitmap[p.x + p.y * bitmap_size] = c;
}

bool nibbles_bot::scan_bitmap(int quadrant_index, const point& start_position, const point& target_position) {
	auto p = start_position.subtract(bitmap_origin);
	auto t = target_position.subtract(bitmap_origin);
	
	std::deque<point> points;
	points.push_back(p);
	while (!points.empty()) {
		const auto& dp = points.front();
		draw_pixel(dp, 2);
		if (scan_pixel(quadrant_index, dp, t, points)) {
			return true;
		}
		points.pop_front();
	}
	return false;
}

bool nibbles_bot::scan_pixel(int quadrant_index, const point& p, const point& target_position, std::deque<point>& points) {

	if (p == target_position) {
		return true;
	}

	for (int i = 0; i < 4; i++) {
		auto tp = p.add(directions[i]);
		if (tp.x < 0 || tp.y < 0 || tp.x >= bitmap_size || tp.y >= bitmap_size) {
			continue;
		}

		auto lp = tp.subtract(bitmap_center);
		auto qi = index_from_point(lp.x, lp.y);
		if (qi != quadrant_index) {
			continue;
		}

		auto  col = get_pixel(tp);
		if (col == 0) {
			draw_pixel(tp, 2);
			points.push_back(tp);
		}
	}
	return false;
	//return result;

}

char nibbles_bot::get_pixel(const point& p) {
	if (p.x < 0 || p.y < 0 || p.x >= bitmap_size || p.y >= bitmap_size) {
		return 0;
	}

	return bitmap[p.x + p.y * bitmap_size];
}

void nibbles_bot::classify_quadrants(nibbles& game, nibbles_player& this_player, std::vector<nibbles_base*>& objects) {

	for (int i = 0; i < 4; i++) {
		auto& quadrant = quadrants[i];
		quadrant.blocked = false;
		quadrant.obstruct = false;
		quadrant.nearest_target = nullptr;
		quadrant.nearest_distance = std::numeric_limits<int>::max();
		quadrant.score = 0;
	}

	int move_direction = this_player.direction_index;
	int opposite_direction = (this_player.direction_index + 2) % 4;

	for (auto viewObject : objects) {
		if (viewObject->type == nibbles_object_type_pixel) {
			nibbles_pixel& pixel = (nibbles_pixel&)*viewObject;
			if (pixel.color != this_player.color && pixel.color != 6) {
				continue; // inedible
			}

			auto qp = pixel.position.subtract(this_player.position);
			auto qi = index_from_point(qp.x, qp.y);
			assert(qi != -1);
			auto& quadrant = quadrants[qi];

			auto dist = this_player.position.distance(pixel.position);
			if (dist <= quadrant.nearest_distance) {
				quadrant.nearest_distance = dist;
				quadrant.nearest_target = &pixel;
			}

			auto distScore = 1 - std::min(dist, 20.0) / 19.0; // clamp/scale dist to 0..1, divide by 19 to give most remote a score of >0
			int colorScore;
			if (pixel.color == this_player.color) {
				colorScore = 3;
			} else {
				colorScore = 1;
			}

			if (qi == move_direction) {
				quadrant.score += (distScore * distScore * distScore * distScore) * colorScore;
			} else if (qi == opposite_direction) {
				quadrant.score += (distScore)* colorScore;
			} else {
				quadrant.score += (distScore * distScore) * colorScore;
			}
		} else if (viewObject->type == nibbles_object_type_player) {

			// two levels of collision: "in which quadrant is this player" og "is there a player in the immediate next cell"
			// we may want to avoid a quadrant where a player is at full force, or its blocked to such a degree we should give up

			for (int j = 0; j < 4; j++) {
				if (colliding_player_body(this_player.position.add(directions[j]), (nibbles_player&)*viewObject)) {
					// obstructed!
					quadrants[j].obstruct = true;
				}
			}
		}
	}

	for (int i = 0; i < 4; i++) {
		auto& quadrant = quadrants[i];

		point quadrant_position;
		if (quadrant.nearest_target != nullptr) {
			quadrant_position = quadrant.nearest_target->position;
		} else {
			// pick a position some units into the quadrant
			quadrant_position = this_player.position.add(directions[i].multiply_scalar(5));
		}

		// obstruct if wall
		point wp = this_player.position.add(directions[i]);
		if (wp.x < 0 || wp.y < 0 || wp.x >= game.width || wp.y >= game.height) {
			quadrant.obstruct = true;
		}

		quadrant.blocked = !scan_bitmap(i, this_player.position, quadrant_position);
	}

}
