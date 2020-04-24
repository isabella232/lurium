#pragma once

struct ortho_matrix {
	glm::mat4 projection;
	float box_left, box_bottom, box_width, box_height;


	// assumes input in -1..1 relative screen units, return pixels position on bitmap
	glm::vec2 unproject(const glm::vec2& p) {
		// scale to 0..1
		auto u = glm::vec2(p.x / 2.0f + 0.5f, p.y / 2.0f + 0.5f);
		// scale to projection
		return glm::vec2(box_left + u.x * box_width, box_bottom + u.y * box_height);
		// scale to bitmap
		//return glm::vec2(v.x * hud_bitmap.width, v.y * hud_bitmap.height);
	}
};

struct matrix_builder {
	glm::mat4 matrix;

	matrix_builder() : matrix(1) {
	}

	matrix_builder& scale(const glm::vec3& value) {
		matrix = glm::scale(matrix, value);
		return *this;
	}

	matrix_builder& translate(const glm::vec3& value) {
		matrix = glm::translate(matrix, value);
		return *this;
	}

	matrix_builder& rotateX(float angle) {
		matrix = glm::rotate(matrix, angle, glm::vec3(1, 0, 0));
		return *this;
	}

	matrix_builder& rotateY(float angle) {
		matrix = glm::rotate(matrix, angle, glm::vec3(0, 1, 0));
		return *this;
	}

	matrix_builder& rotateZ(float angle) {
		matrix = glm::rotate(matrix, angle, glm::vec3(0, 0, 1));
		return *this;
	}
};

