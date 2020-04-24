#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <vector>
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/math/point.h"
#include "nibbles.h"
#include "lurium/math/rand2.h"
#include "lurium/math/pow2.h"

point directions[] = { { 1, 0 },{ 0, 1 },{ -1, 0 },{ 0, -1 } };

bool colliding_player_body(const point& position, const nibbles_player& other_player) {

	auto p1 = other_player.segments_position;
	for (auto& segment : other_player.segments) {
		auto p2 = p1.add(directions[segment.direction].multiply_scalar(segment.length));
		auto minx = std::min(p1.x, p2.x);
		auto miny = std::min(p1.y, p2.y);
		auto maxx = std::max(p1.x, p2.x);
		auto maxy = std::max(p1.y, p2.y);

		if (p1.x == p2.x && p1.x == position.x && position.y >= miny && position.y <= maxy) {
			//console.log("becauseX ", p1.x, p1.y, p2.x, p2.y, minx, maxx, miny, maxy);
			return true;
		} else if (p1.y == p2.y && p1.y == position.y && position.x >= minx && position.x <= maxx) {
			//console.log("becauseY ", p1.x, p1.y, p2.x, p2.y, minx, maxx, miny, maxy, position.x, position.y);
			return true;
		}

		p1 = p2;
	}

	/*	for (auto& p : other_player.body) {
	if (position.x == p.x && position.y == p.y) {
	return true;
	}
	}*/
	return false;
}

bool diffing_quadtree_object_has_changed(nibbles_base* object) {
	if (object->type == nibbles_object_type_pixel) {
		return false;
	}
	if (object->type == nibbles_object_type_player) {
		return true;
	}
	assert(false);
	return false;
}

nibbles::nibbles(int _width, int _height, int _spawn_pixels, int max_pixels, int max_players, bool intersect_self, bool insta_death, int _leave_remains, bool _can_boost) {
	width = _width;
	height = _height;
	can_intersect_self = intersect_self;
	can_insta_death = insta_death;
	leave_remains = _leave_remains;
	can_boost = _can_boost;
	spawn_pixels = _spawn_pixels;
	active_pixels = 0;
	active_players = 0;
	object_counter = 0;
	// start at timestamp 1, so we can spawn pixels in the first tick, all pixels were technically deactivated at timestamp 0
	// and pixels cannnot spawn on the same frame they were deactivated
	last_timestamp = 0;
	timestamp = 1; 

	players.resize(max_players);
	pixels.resize(max_pixels);

	int collider_size = make_pow2(std::max(width, height));
	int depth = get_pow2(collider_size) - get_pow2(16); // want quad of 16x16 at max depth
	assert(depth >= 0);
	collider.reset(collider_size, collider_size, depth);
}

int nibbles::spawn_pixel(int x, int y, int color) {
	// TODO: dont spawn multiple pixels on same location

	int pixel_index = -1;
	for (int i = 0; i < pixels.size(); i++) {
		if (!pixels[i].active && pixels[i].deactivate_timestamp != timestamp) {
			pixel_index = i;
			break;
		}
	}

	if (pixel_index == -1) {
		return -1;
	}

	if (x == -1)
		x = rand2(width);
	if (y == -1)
		y = rand2(height);
	if (color == -1)
		color = rand2(7); // 0-5 player colors, 6 = special color

	nibbles_pixel& pixel = pixels[pixel_index];
	pixel.type = nibbles_object_type_pixel;
	pixel.active = true;
	pixel.object_id = make_object_id();
	pixel.deactivate_timestamp = -1;
	pixel.color = color;
	pixel.position.x = x;
	pixel.position.y = y;

	collider.insert(rect<int>(x, y, x, y), &pixel);

	active_pixels++;
	return pixel_index;
}

bool nibbles::is_near_object(point& p, int radius) {
	rect<int> rc(p.x - radius, p.y - radius, p.x + radius, p.y + radius);
	std::vector<nibbles_base*> objects;
	collider.current.query(rc, objects);

	for (auto& o : objects) {
		switch (o->type) {
			case nibbles_object_type_pixel:
				// continue instead of return to avoid bias against areas with lots of pixels
				// -> fix: if there is a pixel here, find nearest postition instead of retry
				continue;
				//return true;
			case nibbles_object_type_player:
				// player bounds intersect with query rect - check the entire body
				if (colliding_player_body(p, (nibbles_player&)*o)) {
					return true;
				}
				/*for (auto& body : o.player->body) {
					auto d = body.distance(p);
					if (d < radius) {
						return true;
					}
				}*/
				break;
			default:
				assert(false);
				break;
		}
	}
	return false;
}

int nibbles::spawn_player(const std::string& nick) {
	// pick a random location 5 px away from the edges and accept if its more than 3px away from any player's body. 
	// populate body with 1 pixel to avoid bounds issues if still spawning in a bad location.
	// set grow to 1, so that the initial length is 2, the minimum needed for a turtle segment.

	int player_index = -1;
	for (int i = 0; i < (int)players.size(); i++) {
		if (!players[i].active) {
			player_index = i;
			break;
		}
	}

	if (player_index == -1) {
		return -1;
	}

	nibbles_player& player = players[player_index];
	player.type = nibbles_object_type_player;

	int attempts = 0;
	do {
		// spawn somewhat away from the edges
		player.position.x = rand2(width - 10) + 5;
		player.position.y = rand2(height - 10) + 5;
		attempts++;
		if (attempts > 1000) {
			return -1;
		}
	} while (is_near_object(player.position, 3));

	player.nick = nick;
	player.color = rand2(6);
	player.press_space = false;
	player.direction_index = rand2(4);
	player.segments.clear();
	player.segments.push_back(turtle_segment());
	player.segments.back().direction = (player.direction_index + 2) % 4; // opposite of diretion_index
	player.segments.back().length = 1;
	player.segments_position = player.position;
	player.length = 2;
	player.grow = 0;
	player.active = true;
	player.object_id = make_object_id();
	player.deactivate_timestamp = -1;

	update_bounds(player);

	collider.insert(player.bounds, &player);
	active_players++;
	return player_index;
}

void nibbles::remove_player(nibbles_player& player) {
	collider.remove(player.bounds, &player);

	player.active = false;
	player.deactivate_timestamp = timestamp;

	active_players--;

	if (leave_remains > 0) {

		int spawn_counter = 0;
		// spawn white pixels all over body TODO: option for less spawning
		auto p1 = player.segments_position;
		for (auto& segment : player.segments) {

			for (int i = 0; i < segment.length; ++i) {
				if ((spawn_counter % leave_remains) == 0) {
					spawn_pixel(p1.x, p1.y, 6);
				}
				p1 = p1.add(directions[segment.direction]);
				spawn_counter++;
			}

		}
	}
}

void nibbles::set_input(nibbles_player& player, int direction_index, bool press_space) {
	player.direction_index = direction_index;
	player.press_space = press_space;
}

void nibbles::move_player(nibbles_player& player) {

	int opposite_direction = player.segments.front().direction;
	if (player.direction_index == opposite_direction) {
		// dont move in opposite direction
		player.direction_index = (opposite_direction + 2) % 4;
	}

	const point& velocity = directions[player.direction_index];

	player.position.x = player.segments_position.x + velocity.x;
	player.position.y = player.segments_position.y + velocity.y;
}

void check_walls(nibbles_player& player, int width, int height) {
	for (int i = 0; i < 4; i++) {
		if (player.collides[i].collide) {
			continue;
		}

		// check if other player will move its head to this location and indicate in matrix as well
		point next_position = player.segments_position.add(directions[i]);
		if ((next_position.x < 0 || next_position.x >= width) || (next_position.y < 0 || next_position.y >= height)) {
			player.collides[i].collide = true;
			player.collides[i].collide_type = collision_type_wall;
		}
	}
}

void check_directions(nibbles_player& player, nibbles_player& other_player) {
	for (int i = 0; i < 4; i++) {
		if (player.collides[i].collide) {
			continue;
		}

		// check if other player will move its head to this location and indicate in matrix as well
		point next_position = player.segments_position.add(directions[i]);
		if (&player != &other_player && next_position == other_player.position) {
			player.collides[i].collide = true;
			player.collides[i].collide_type = collision_type_head;
			player.collides[i].player = &other_player;
		} else if (colliding_player_body(next_position, other_player)) {
			player.collides[i].collide = true;
			player.collides[i].collide_type = collision_type_tail;
			player.collides[i].player = &other_player;
		}
	}
}

void nibbles::collide_player(nibbles_player& player) {

	player.collides[0].collide = false;
	player.collides[1].collide = false;
	player.collides[2].collide = false;
	player.collides[3].collide = false;

	check_walls(player, width, height);

	if (!can_intersect_self) {
		check_directions(player, player);
	}

	// the collider currently has rects for the previous frame, so we match an extra pixel around the head
	rect<int> head(player.position.x - 1, player.position.y - 1, player.position.x + 1, player.position.y + 1);
	std::vector<nibbles_base*> objects;
	collider.current.query(head, objects);

	for (auto& object : objects) {
		if (object->type == nibbles_object_type_player) {
			
			auto& other_player = (nibbles_player&)*object;
			if (&other_player == &player) {
				continue;
			}

			check_directions(player, other_player);

			// two kinds of head-to-head collision:
			// ***] [***  both snakes move to the same slot
			// ***][***   both snakes move inside each others body

		} else if (object->type == nibbles_object_type_pixel) {
			nibbles_pixel& pixel = (nibbles_pixel&)*object;
			if (pixel.position == player.position) {
				if (pixel.color == 6) {
					player.grow += 1;
				} else if (pixel.color == player.color) {
					player.grow += 3;
				} else {
				 // TODO: negative grow, this is different from colliding
					//player.collide = true;
					//player.collide_type = collision_type_pixel;
					//player.grow -= 1;
				}

				rect<int> rc(pixel.position.x, pixel.position.y, pixel.position.x, pixel.position.y);
				collider.remove(rc, object);
				pixel.active = false;
				pixel.deactivate_timestamp = timestamp;
				active_pixels--;
			}
		}
	}
}

void nibbles::update_bounds(nibbles_player& player) {

	player.bounds.left = std::numeric_limits<int>::max();
	player.bounds.top = std::numeric_limits<int>::max();
	player.bounds.right = std::numeric_limits<int>::min();
	player.bounds.bottom = std::numeric_limits<int>::min();

	auto p1 = player.segments_position;
	player.bounds.extend_bound(p1.x, p1.y);

	for (auto& segment : player.segments) {
		auto p2 = p1.add(directions[segment.direction].multiply_scalar(segment.length));

		player.bounds.extend_bound(p2.x, p2.y);
		p1 = p2;
	}
}

void nibbles::update_player(nibbles_player& player) {

	bool collide = player.collides[player.direction_index].collide;
	bool all_collide = 
		player.collides[0].collide &&
		player.collides[1].collide &&
		player.collides[2].collide &&
		player.collides[3].collide;

	if (collide) {
		player.position = player.segments_position;

		if (all_collide || can_insta_death) {
			remove_player(player);
			return;
		}

		player.length--;

	} else {
		// extend front segment if same(opposite) direction, or create new segment
		int opposite_direction = (player.direction_index + 2) % 4;
		
		if (player.segments.front().direction == opposite_direction) {
			player.segments.front().length++;
		} else {
			turtle_segment segment;
			segment.direction = opposite_direction;
			segment.length = 1;
			player.segments.insert(player.segments.begin(), segment);
		}
		player.segments_position = player.position;
	}

	if (player.grow > 0) {
		player.grow--;
		player.length++;
	} else {
		// shorten the tail segment, or trim if length==2
		if (player.segments.back().length == 1) {
			player.segments.pop_back();
		} else {
			assert(player.segments.back().length >= 1);
			player.segments.back().length--;
		}

		if (player.grow < 0) {
			player.grow++;
			assert(false); // length stuff etc changed
		}
	}

	if (player.segments.empty()) {
		// ded!
		remove_player(player);
		return;
	} 

	// TODO: collider.move()
	collider.remove(player.bounds, &player);

	update_bounds(player);
	collider.insert(player.bounds, &player);

}

void nibbles::tick() {

	// check if tick_done was called
	assert(last_timestamp != timestamp);

	// boosting
	if (can_boost) {
		bool boosted = false;
		for (auto& player : players) {
			if (!player.active || !player.press_space) {
				continue;
			}

			move_player(player);
			boosted = true;
		}

		if (boosted) {
			// update/collide if anybody boosted
			for (auto& player : players) {
				if (!player.active || !player.press_space) {
					continue;
				}
				collide_player(player);
			}

			for (auto& player : players) {
				if (!player.active || !player.press_space) {
					continue;
				}
				update_player(player);
			}
		}
	}

	// update normally
	for (auto& player : players) {
		if (!player.active) {
			continue;
		}

		move_player(player);
	}

	for (auto& player : players) {
		if (!player.active) {
			continue;
		}
		collide_player(player);
	}

	for (auto& player : players) {
		if (!player.active) {
			continue;
		}
		update_player(player);
	}

	if (active_pixels < spawn_pixels) {
		spawn_pixel();
	}

	// now caller can create a view diff, then call tick done
	last_timestamp = timestamp;
}

void nibbles::tick_done() {
	collider.apply();
	timestamp++;
}
