#pragma once

struct ground_generator {
	int max_results;
	randomgenerator random;
	int random_seed;

	int max_height;
	int max_length;
	//int segment_index;
	int segment_position;
	int segment_length;
	int interpolate_type;
	int end_height;
	int start_height;

	ground_generator(int seed, int max_terrain_height, int max_segment_length, int max_terrain_results) {
		max_height = max_terrain_height;
		max_length = max_segment_length;
		random_seed = seed;
		max_results = max_terrain_results;
		reset();
	}

	void reset() {
		//segment_index = 0;
		segment_position = 0;
		segment_length = 0;

		random = randomgenerator(random_seed);
		end_height = rand3(max_height);
		next_segment();
	}

	void next_segment() {
		segment_length = rand3(max_length) + 10;
		//segment_length = std::min(segment_length, max_results - segment_index);

		start_height = end_height;
		end_height = rand3(max_height);

		// smooth, linear, flat
		interpolate_type = rand3(3);
	}

	int next() {
		/*if (segment_index >= max_results) {
			return -1;
		}*/

		if (segment_position >= segment_length) {

			next_segment();

			segment_position = 0;
			//segment_index++;
		}

		int y;
		if (interpolate_type == 0) {
			y = (int)CosineInterpolate(start_height, end_height, segment_position / (float)segment_length);;
		} else if (interpolate_type == 1) {
			y = (int)LinearInterpolate(start_height, end_height, segment_position / (float)segment_length);;
			//s.y2 = start_height + (delta * i);
		} else if (interpolate_type == 2) {
			y = end_height;
		}

		segment_position++;
		return y;
	}

private:
	int rand3(int range) {
		return (int)(random.next() * range);
	}

	// http://paulbourke.net/miscellaneous/interpolation/
	double CosineInterpolate(double y1, double y2, double mu) {
		double mu2;

		mu2 = (1 - std::cos(mu * M_PI)) / 2;
		return(y1*(1 - mu2) + y2*mu2);
	}

	double LinearInterpolate(double y1, double y2, double mu) {
		return(y1*(1 - mu) + y2*mu);
	}

};

struct ground_segment {
	float y1;
	float y2;
	int fall_delay;
	bool falling;
	float velocity;
};

struct scorch_ground {
	std::vector<ground_segment> segments;
};


struct terrain_ground {

	std::vector<scorch_ground> ground;

	int get_height_at(int x) {
		auto& g = ground[x];
		if (g.segments.empty()) {
			return 0;
		}

		return (int)g.segments.back().y2;
	}


	void cut_terrain(int x, int y1, int y2) {
		if (y1 == y2) return;
		if (x < 0) return;
		if (x >= (int)ground.size()) return;

		auto& ground = this->ground[x];

		// the segments must be space between y..(y+height)
		for (auto i = 0; i < (int)ground.segments.size(); ) {
			auto& segment = ground.segments[i];
			if (y1 > segment.y2) {
				i++;
				continue; // fully after
			}
			else if (y2 < segment.y1) {
				i++;
				continue; // fully before
			}

			if (y1 > segment.y1 && y2 < segment.y2) {
				// fully inside, cut off, and create new at end
				// flag the inserted segment for falling some time ahead! also any following segments should be flagged, (unless they are already, or we can treat everythhing following as falling)
				ground_segment insert_segment;
				insert_segment.y1 = (float)y2;
				insert_segment.y2 = segment.y2;
				insert_segment.falling = true;
				insert_segment.fall_delay = 0;
				insert_segment.velocity = 0;
				segment.y2 = (float)y1;
				ground.segments.insert(ground.segments.begin() + i + 1, insert_segment);
				i += 2;
			}
			else if (y1 <= segment.y1 && y2 >= segment.y2) {
				// fully outside, remove
				ground.segments.erase(ground.segments.begin() + i);
			}
			else if (y1 <= segment.y1 && y2 < segment.y2) {
				// overlap start, trim
				segment.y1 = (float)y2;
				segment.falling = true;
				segment.fall_delay = 0;
				i++;
			}
			else if (y1 > segment.y1 && y2 >= segment.y2) {
				// overlap end, trim
				segment.y2 = (float)y1;
				i++;
			}
			else {
				assert(false); // Unhandled clipping case
			}
		}

	}

	/*
	*/

	void grow_terrain(int x, int y1, int y2) {
		if (y1 == y2) return;
		if (x < 0) return;
		if (x >= (int)ground.size()) return;

		auto& ground = this->ground[x];

		ground_segment* last_segment = NULL;

		for (auto i = 0; i < (int)ground.segments.size(); ) {
			auto& segment = ground.segments[i];

			last_segment = &segment;

			if (y1 > segment.y2) {
				i++;
				continue; // fully after, try next
			}
			else if (y2 < segment.y1) {
				ground_segment insert_segment;
				insert_segment.y1 = y1;
				insert_segment.y2 = y2;
				insert_segment.falling = true;
				insert_segment.fall_delay = 0;
				insert_segment.velocity = 0;
				ground.segments.insert(ground.segments.begin() + i + 1, insert_segment);
				return; // fully before, inserte
			}
			else if (y1 > segment.y1 && y2 < segment.y2) {
				// fully inside - ok
				return;
			}
			else if (y1 <= segment.y1 && y2 >= segment.y2) {
				// fully outside - replace??
				// ie, delete, then call grow again
				ground.segments.erase(ground.segments.begin() + i);
				grow_terrain(x, y1, y2);
				return;
			}
			else if (y1 <= segment.y1 && y2 < segment.y2) {
				// overlap start, replace with larger
				y2 = segment.y2;
				ground.segments.erase(ground.segments.begin() + i);
				grow_terrain(x, y1, y2);// segment.y2);
				return;
			}
			else if (y1 > segment.y1 && y2 >= segment.y2) {
				// overlap end, replace with larger
				y1 = segment.y1;
				ground.segments.erase(ground.segments.begin() + i);
				grow_terrain(x, y1, y2);// segment.y1, y2);
				return;
			}
			else {
				assert(false); // Unhandled grow case
			}

		}

		if (last_segment == NULL || y2 > last_segment->y1) {
			ground_segment insert_segment;
			insert_segment.y1 = y1;
			insert_segment.y2 = y2;
			insert_segment.falling = true;
			insert_segment.fall_delay = 0;
			insert_segment.velocity = 0;
			ground.segments.push_back(insert_segment);
		}

	}

	void modify_terrain(int x, int y, int radius, bool is_dirt) {

		for (int i = -radius; i <= radius; i++) {
			int height = std::sqrt(radius * radius - i * i);
			int y1 = y - height;
			int y2 = y + height;
			if (is_dirt) {
				grow_terrain(x + i, y1, y2);
			}
			else {
				cut_terrain(x + i, y1, y2);
			}
		}

	}

	void process(float gravity_y) {
		for (auto& g : this->ground) {
			process_ground(g, gravity_y);
		}
	}

	void process_ground(scorch_ground& ground, float gravity_y) {

		// TODO: instead of enumerating every ground, we could put out and query for "fallers" in the collider, which are regions where terrain falling occurs, ie, in the diameter of an explosion

		auto fall_height = 0.0f;
		auto falling = false;
		auto velocity = 0.0f;

		for (auto i = 0; i < ground.segments.size(); ) {
			auto& segment = ground.segments[i];

			if (!falling && segment.falling && segment.fall_delay == 0) {
				falling = true;
				velocity = segment.velocity + gravity_y;
				segment.velocity = velocity;
			}

			if (falling) {
				if (segment.y1 + velocity <= fall_height) {
					auto fall_velocity = fall_height - segment.y1;
					segment.y1 += fall_velocity;
					segment.y2 += fall_velocity;

					if (i > 0) {
						// Merge if falling onto another segment
						auto y2 = segment.y2;
						ground.segments[i - 1].y2 = y2;
						fall_height = y2;
						// NOTE: segment becomes invalid after erase()
						ground.segments.erase(ground.segments.begin() + i);
					}
					else {
						// Settle on ground
						assert(segment.y1 == 0.0f);
						segment.falling = false;

						fall_height = segment.y2;
						i++;
					}

				}
				else {
					segment.y1 += velocity;
					segment.y2 += velocity;
					assert(segment.y1 > fall_height); // SHULD BE FIXED: afaict this could happen , also falling is never set to false

					fall_height = segment.y2;
					i++;
				}
			}
			else {
				if (segment.fall_delay > 0) {
					segment.fall_delay--;
				}

				fall_height = segment.y2;
				i++;
			}

		}

	}


};