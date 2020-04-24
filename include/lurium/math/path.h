#pragma once


struct path_builder {
	std::vector<std::vector<glm::vec2>> polygon;
	std::vector<glm::vec2>& path;

	path_builder(const glm::vec2& origin) : polygon{ { origin } }, path(polygon.front()) {
	}

	void line_to(const glm::vec2& p) {
		path.push_back(p);
	}

	void arc(int points, float radius, float start_angle, float end_angle) {
		assert(points >= 1);
		glm::vec2 translate = path.back();
		glm::vec2 start(radius * cos(start_angle), radius * sin(start_angle));

		auto delta = (end_angle - start_angle) / points;
		auto angle = start_angle + delta;
		for (auto i = 0; i < points; i++) {
			glm::vec2 p(radius * cos(angle), radius * sin(angle));
			line_to(translate + (p - start));
			angle += delta;
		}
	}

	void rect(float width, float height) {
		auto p = path.back();

		line_to(glm::vec2(p.x + width, p.y));
		line_to(glm::vec2(p.x + width, p.y + height));
		line_to(glm::vec2(p.x, p.y + height));
		line_to(glm::vec2(p.x, p.y));
	}

	void round_rect(float width, float height, float radius) {
		// starts on p: the * are radius big
		// *p----*
		// |     |
		// *-----*

		auto p = path.back() + glm::vec2(-radius, 0);
		auto r = radius;
		auto r2 = radius * 2;

		line_to(glm::vec2(p.x + width - r, p.y));
		arc(3, radius, -M_PI / 2, 0);

		line_to(glm::vec2(p.x + width, p.y + height - r));
		arc(3, radius, 0, M_PI / 2);

		line_to(glm::vec2(p.x + r, p.y + height));
		arc(3, radius, M_PI / 2, M_PI);

		line_to(glm::vec2(p.x, p.y + r));
		arc(3, radius, M_PI, M_PI + M_PI / 2);
	}
};
