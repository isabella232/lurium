#pragma once

struct bot_quadrant {
	nibbles_pixel* nearest_target;
	double nearest_distance;
	double score;
	bool obstruct;
	bool blocked;
};

struct nibbles_bot {
	int client_id;
	int last_force;
	point seek_position;
	std::vector<char> bitmap;
	point bitmap_origin;
	point bitmap_center;
	int bitmap_size;
	bot_quadrant quadrants[4];

	nibbles_bot();
	void update(nibbles& game);
	void post_update(nibbles& game);

	void update_bitmap(int radius, const point& origin, std::vector<nibbles_base*>& objects);

	void draw_player(nibbles_player& player);
	void draw_pixel(const point& p, char c);
	char get_pixel(const point& p);

	bool scan_bitmap(int quadrant_index, const point& start_position, const point& target_position);
	bool scan_pixel(int quadrant_index, const point& p, const point& target_position, std::deque<point>& points);

	void classify_quadrants(nibbles& game, nibbles_player& this_player, std::vector<nibbles_base*>& objects);
	void select_target(nibbles_player& this_player);
};
