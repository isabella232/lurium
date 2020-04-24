#pragma once

struct trajectory {

	static bool height_at(float Yo, float Vx, float Vy, float Ge, float x, float* height) {
		float theta = atan2(Vy, Vx);
		float v = glm::length(glm::vec2(Vx, Vy));// 0; // velocity ? length of vx , vy

		float a = v * cos(theta);

		*height = Yo + x * tan(theta) - ((Ge * x * x) / (2 * a * a));

		return true;
	}

	// ah, vi kan ta trajectory box, og så height at x

	static bool bounding_box(float Yo, float Vx, float Vy, float Ge, float* xmax, float *ymax) {
		// http://www.convertalot.com/ballistic_trajectory_calculator.html
		auto hgt = Yo + Vy*Vy / (2 * Ge);          // max height

		if (hgt < 0.0) return false;

		auto upt = Vy / Ge;                      // time to max height
		auto dnt = std::sqrt(2 * hgt / Ge);        // time from max height to impact
		auto rng = Vx*(upt + dnt);             // horizontal range at impact

											  // flight time to impact, speed at impact
		//auto imp = upt + dnt;
		//auto spd = Math.sqrt((Ge*dnt)*(Ge*dnt) + Vx*Vx);

		*xmax = rng;
		*ymax = hgt;
		return true;
		/*return{
		ymax: hgt,
			  xmax : rng

		};*/

	}

};
