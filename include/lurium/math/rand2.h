#pragma once

// random without modulus bias

static inline int rand2(int range) {
	while (true) {
		int value = rand();
		if (value < RAND_MAX - RAND_MAX % range)
			return value % range;
	}
}
