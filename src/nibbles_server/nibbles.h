#pragma once

enum collision_type {
	collision_type_wall,
	collision_type_head,
	collision_type_tail,
	collision_type_self,
	//collision_type_pixel
};

enum nibbles_object_type {
	nibbles_object_type_pixel,
	nibbles_object_type_player
};

struct nibbles_base {
	nibbles_object_type type;
	int object_id;
	bool active;
	int deactivate_timestamp;
};

struct nibbles_player;

struct direction_collision {
	bool collide;
	collision_type collide_type;
	nibbles_player* player;
};

struct turtle_segment {
	int direction;
	int length;
};

struct nibbles_player : nibbles_base {
	std::string nick;
	point position;
	int color;
	int length;
	int direction_index;
	bool press_space;
	int grow;
	point segments_position;
	std::vector<turtle_segment> segments;
	rect<int> bounds;
	direction_collision collides[4];
};

struct nibbles_pixel : nibbles_base {
	point position;
	char color;
};

bool diffing_quadtree_object_has_changed(nibbles_base* object);

struct nibbles {
	int width;
	int height;
	bool can_intersect_self;
	bool can_insta_death;
	bool can_boost;
	int leave_remains;
	int spawn_pixels;
	int active_pixels;
	int active_players;
	int object_counter;
	int timestamp;
	int last_timestamp;

	std::vector<nibbles_player> players;
	std::vector<nibbles_pixel> pixels;
	diffing_quadtree<int, nibbles_base*> collider;

	nibbles(int width, int height, int spawn_pixels, int max_pixels, int max_players, bool can_intersect_self, bool insta_death, int _leave_remains, bool booost);

	int make_object_id() { object_counter++; return object_counter - 1; }
	int spawn_pixel(int x = -1, int y = -1, int color = -1);
	int spawn_player(const std::string& nick);
	void remove_player(nibbles_player& player);
	void set_input(nibbles_player& player, int direction_index, bool press_space);
	bool is_near_object(point& p, int radius);

	void tick();
	void move_player(nibbles_player& player);
	void collide_player(nibbles_player& player);
	void update_player(nibbles_player& player);
	void update_bounds(nibbles_player& player);

	void tick_done();
};
