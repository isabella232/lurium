#pragma once

// same implementation as "random.js"
struct randomgenerator {
	int w;
	int z;

	randomgenerator() {}

	randomgenerator(const randomgenerator& rhs) {
		this->w = rhs.w;
		this->z = rhs.z;
	}

	randomgenerator(int seed) {
		this->w = seed;
		this->z = 987654321;
	}

	float next() {
		/*unsigned int mask = 0xffffffff;*/
		this->z = (36969 * (this->z & 65535) + (this->z >> 16))/* & mask*/;
		this->w = (18000 * (this->w & 65535) + (this->w >> 16))/* & mask*/;
		float result = (float)(int)((this->z << 16) + this->w) /*& mask*/;
		result /= 4294967296.0f;
		return result + 0.5f;
	}
};

