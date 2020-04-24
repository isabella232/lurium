#pragma once

#include "earcut.hpp"
#include "lurium/math/path.h"
#include "lurium/gl/path.h"
#include "lurium/math/matrix_builder.h"
#include "lurium/math/trajectory.h"
#include "lurium/math/rand2.h"
#include "scorch_state.h"
#include "arrow_buttons_view.h"
#include "weapon_view.h"

// ground_shader renders vertical terrain lines; TODO: gradient, also use for sky background!
struct ground_shader : shader {
	GLint uProjection;
	GLint uModel;
	GLint uMinY;
	GLint uMaxY;
	GLint uMinColor;
	GLint uMaxColor;

	ground_shader() {
		const char vShaderStr[] =
			"precision mediump float;\n"
			"attribute vec3 aPosition;\n"
			"uniform mat4 uProjection;\n"
			"uniform mat4 uModel;\n"
			"uniform float uMinY;\n"
			"uniform float uMaxY;\n"
			"varying float vY;\n"

			"void main() {\n"
			"  gl_Position = uProjection * uModel * vec4(aPosition, 1.0);\n"
			"  float modelY = (uModel * vec4(aPosition, 1.0)).y;\n"
			"  vY = (modelY - uMinY) / (uMaxY - uMinY);\n"
			"}\n";

		const char fShaderStr[] =
			"precision mediump float;\n"
			"uniform vec3 uMinColor;\n"
			"uniform vec3 uMaxColor;\n"
			"varying float vY;\n"

			"void main() {\n"
			"  gl_FragColor = vec4((uMinColor * vY) + (uMaxColor * (1.0 - vY)), 1.0);\n" //( vec4(1.0, vY, 0.1, 1.0);\n"
			"}\n";

		create_program(vShaderStr, fShaderStr);

		glBindAttribLocation(programObject, 0, "aPosition");

		link_program();

		uProjection = glGetUniformLocation(programObject, "uProjection");
		uModel = glGetUniformLocation(programObject, "uModel");
		uMinY = glGetUniformLocation(programObject, "uMinY");
		uMaxY = glGetUniformLocation(programObject, "uMaxY");
		uMinColor = glGetUniformLocation(programObject, "uMinColor");
		uMaxColor = glGetUniformLocation(programObject, "uMaxColor");
	}

	void bind(float minY, float maxY, const glm::vec3& minColor, const glm::vec3& maxColor, const glm::mat4& projection, const glm::mat4& model) {
		glUseProgram(programObject);
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(uMinY, minY);
		glUniform1f(uMaxY, maxY);
		glUniform3fv(uMinColor, 1, glm::value_ptr(minColor));
		glUniform3fv(uMaxColor, 1, glm::value_ptr(maxColor));
	}

};

struct scorch_view : ui_element {
	bitmap_font& normal_font;
	scorch_state& state;

	glm::mat4 projection;
	int screen_width, screen_height;

	quad_buffer line_mesh;
	ground_shader line_material;

	path_line_shader shape_outline_shader;
	shader_plain shape_fill_shader;
	shape_node body_shape;
	shape_node turret_shape;
	shape_node bullet_shape;
	shape_node explosion_shape;
	shape_node bonus_shape;

	progress_shape health_bar;
	path_line_buffer trajectory_mesh;

	shape_node parallax1, parallax2, cloud1;

	ui_context* parent_context;

	scorch_view(scorch_state& viewstate, ui_context* parent, bitmap_font& font)
		: parent_context(parent)
		, state(viewstate)
		, normal_font(font)
		, shape_fill_shader(false, true, false, material_light::nolight())
		, body_shape(shape_fill_shader, shape_outline_shader)
		, turret_shape(shape_fill_shader, shape_outline_shader)
		, bullet_shape(shape_fill_shader, shape_outline_shader)
		, explosion_shape(shape_fill_shader, shape_outline_shader)
		, bonus_shape(shape_fill_shader, shape_outline_shader)
		, parallax1(shape_fill_shader, shape_outline_shader)
		, parallax2(shape_fill_shader, shape_outline_shader)
		, cloud1(shape_fill_shader, shape_outline_shader)
		, health_bar(shape_fill_shader, shape_outline_shader, 16, 3.0f, 1.0f, 0.5f)

	{
		assert(parent_context != NULL);
		parent_context->elements.push_back(this);

		line_mesh.create([](const glm::vec3& v) { return glm::vec3(v.x + 0.5, v.y + 0.5, v.z); }, [](const glm::vec2& v) { return v; });

		// create tank meshes in unit size; scale when drawing
		path_builder tank_path(glm::vec2(1, 0)); // set origin half radius to the side, draw half circle around center
		tank_path.arc(10, 1, 0, M_PI);
		tank_path.line_to(glm::vec2(1, 0));

		body_shape.create(tank_path);
		body_shape.polygon_material.material_color = glm::vec3(0.2, 0.8, 0.2);
		body_shape.thickness = 0.75f;// / tank_radius;


		auto barrel_width_angle = 20.0f / 180.0f * M_PI;

		glm::vec2 barrel_start(glm::vec2(cos(-barrel_width_angle / 2), sin(-barrel_width_angle / 2)) * 0.5f);
		glm::vec2 barrel_end(glm::vec2(cos(barrel_width_angle / 2), sin(barrel_width_angle / 2)) * 0.5f);
		//path_builder barrel_path(glm::vec2(0, 0));
		path_builder barrel_path(barrel_start);

		barrel_path.line_to(glm::vec2(cos(-barrel_width_angle/2), sin(-barrel_width_angle/2)));
		barrel_path.arc(3, 1, -barrel_width_angle/2, barrel_width_angle/2);
		barrel_path.line_to(barrel_end);
		barrel_path.line_to(barrel_start);
		//barrel_path.line_to(glm::vec2(0, 0));

		turret_shape.create(barrel_path);
		turret_shape.polygon_material.material_color = glm::vec3(0.5, 0.5, 0.5);
		turret_shape.thickness = 0.75f;


		path_builder circle_path(glm::vec2(1, 0));
		circle_path.arc(10, 1, 0, M_PI * 2);

		bullet_shape.polygon_material.material_color = glm::vec3(0.6f, 0.6f, 0.6f);
		bullet_shape.thickness = 0.75f;
		bullet_shape.create(circle_path);

		explosion_shape.polygon_material.material_color = glm::vec3(1, 1, 0);
		explosion_shape.thickness = 0.75f;
		explosion_shape.create(circle_path);
		
		glm::vec2 start_p(0.0f, 1.0f);
		path_builder bonus_path(start_p);
		float start_angle = atan2(start_p.y, start_p.x);
		int arms = 5;
		for (int i = 1; i < (arms * 2); i++) {
			if ((i % 2) == 1) {
				float angle = start_angle + (float)M_PI / (arms) * (i);
				float x = cos(angle) * 0.5f;
				float y = sin(angle) * 0.5f;
				bonus_path.line_to(glm::vec2(x, y));
			}
			else {
				float angle = start_angle + (float)M_PI / (arms) * (i);
				float x = cos(angle) * 1.0f;
				float y = sin(angle) * 1.0f;
				bonus_path.line_to(glm::vec2(x, y));
			}
		}
		bonus_path.line_to(start_p);
		bonus_shape.create(bonus_path);

		// TODO: suspect terrain is slow on androids, can we make many smaller ones instead; put in a quadtree or some algo, can have clouds too
		// scenery quadtree, rect + shape; clouds and terrains, differnt colors, .. blabla
		// parallax = big background polygon w single color terrain
		// size max width div parallax speed; TODO: regenerate on welcome and use arena_size
		path_builder tb1(glm::vec2(0, 0));
		float x = 0;
		while (x < 10000) {
			int height = rand2(100);
			tb1.line_to(glm::vec2(x, height));
			x += 50 + rand2(150);
		}
		tb1.line_to(glm::vec2(x, 0));
		tb1.line_to(glm::vec2(0, 0));
		parallax1.polygon_material.material_color = glm::vec3(0.5f, 0.8f, 0.4f);
		parallax1.create(tb1);

		path_builder tb2(glm::vec2(0, 0));
		x = 0;
		while (x < 10000) {
			int height = 50 + rand2(200);
			tb2.line_to(glm::vec2(x, height));
			x += 25 + rand2(75);
		}
		tb2.line_to(glm::vec2(x, 0));
		tb2.line_to(glm::vec2(0, 0));
		parallax2.polygon_material.material_color = glm::vec3(0.7f, 0.8f, 0.4f);
		parallax2.create(tb2);

		// TOOD: clouds? 
		path_builder cb1(glm::vec2(0, 0));

		cb1.line_to(glm::vec2(100, 0));

		// create bubbly arcs in a circlush shape, random start and end angles in such ranges they form a cloudish, sub-arcs gave less than 180 degrees, more than 90 degrees
		float angle = 0;
		start_angle = 0;// M_PI;// +M_PI / 2.0f;
		std::vector<std::tuple<float, float>> bubbles = { { 0.5f, 5.0f }, { 0.4f, 10.0f }, { 0.5f, 20.0f }, { 0.5f, 20.0f }, { 0.3f, 10.0f }, { 0.1f, 15.0f } };
		for (auto bubble : bubbles) {
			float subangles = M_PI / 4.0f + std::get<0>(bubble) * M_PI / 4.0f;
			float radius = std::get<1>(bubble);
			cb1.arc(5, radius, start_angle + angle - subangles, start_angle + angle + subangles);
			angle += subangles / 2.0;// M_PI / 4;
		}

		cb1.line_to(glm::vec2(0, 0));
		cloud1.create(cb1);
		cloud1.thickness = 0.5f;
		cloud1.polygon_material.material_color = glm::vec3(1.0f, 1.0f, 1.0f);

		// create a dummy trajectory
		path_builder trajectory_path(glm::vec2(0, 0));
		trajectory_path.rect(1, 1);
		trajectory_mesh.create(trajectory_path);
	}

	~scorch_view() {
		auto i = std::find(parent_context->elements.begin(), parent_context->elements.end(), this);
		assert(i != parent_context->elements.end());
		parent_context->elements.erase(i);
	}

	virtual bool lbuttonup(const mouse_event_data& e) {
		if (parent_context->get_root()->get_capture(e.id) == this) {
			parent_context->get_root()->set_capture(e.id, NULL);
		}
		return false;
	}

	virtual bool lbuttondown(const mouse_event_data& e) {
		parent_context->get_root()->set_capture(e.id, this);
		return true;
	}

	virtual bool mousemove(const mouse_event_data& e) {
		//  need to project the player position to get the correct zoom
		auto pos = projection * glm::vec4(state.player_position, 0, 1);
		float x = e.x;
		float y = e.y + pos.y;
		state.mouse_angle = -atan2(y, x);
		state.power = glm::length(glm::vec2(x, y)) * 1000.0f;
		return false;
		
	}

	virtual bool keydown(const keyboard_event_data& e) {
		return false;
	}

	virtual bool keyup(const keyboard_event_data& e) {
		return false;
	}

	virtual void resize(const window_event_data& e) {
		screen_width = e.width;
		screen_height = e.height;

	}

	float zoom_width;
	float zoom_top;
	float zoom_left;

	void render() {
		glViewport(0, 0, screen_width, screen_height);
		glClearColor(0.7f, 0.7f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		auto ratio = screen_height / (float)screen_width;
		zoom_left = 0;
		zoom_width = state.terrain_width;
		zoom_top = state.terrain_width * ratio;
		if (screen_width < state.terrain_width) {
			auto diff = (state.terrain_width - screen_width);
			zoom_left = diff / 2;
			zoom_width = screen_width;
			zoom_top = screen_width * ratio;
		}

		if (screen_height > state.arena_height) {
			ratio = screen_width / (float)screen_height;
			zoom_top = state.arena_height;
			zoom_width = state.arena_height * ratio;
			zoom_left = (state.terrain_width - zoom_width) / 2.0f;
		}

		// TODO: also scale based on arena_height; because mobile in portrait mode is unplayable

		projection = glm::ortho<float>(zoom_left, zoom_left + zoom_width, 0, zoom_top, 0, 10);

		render_parallax();
		render_ground();

		for (auto& player : state.players) {
			render_tank(player);
		}

		for (auto& bullet : state.bullets) {
			render_bullet(bullet);
		}

		for (auto& explosion : state.explosions) {
			render_explosion(explosion);
		}

		for (auto& bonus : state.bonuses) {
			render_bonus(bonus);
		}

		render_trajectory();
	}

	void render_parallax() {
		matrix_builder cm;
		cm.translate(glm::vec3(140, 140, 0));
		cloud1.render(projection, cm.matrix);

		matrix_builder m2;
		m2.translate(glm::vec3(-state.terrain_left / 3.0f, 0, 0));
		parallax2.render_shape(projection, m2.matrix);

		matrix_builder m1;
		m1.translate(glm::vec3(-state.terrain_left / 2.0f, 0, 0));
		parallax1.render_shape(projection, m1.matrix);

	}

	void render_trajectory() {

		// TODO: max_power from server
		const float max_power = 400 / state.frame_rate;

		glm::vec2 direction_vector(cos(state.mouse_angle), sin(state.mouse_angle));
		glm::vec2 position = state.player_position + direction_vector * (float)state.barrel_radius;

		float velo_x = cos(state.mouse_angle) * state.power / 1000.0f * max_power;
		float velo_y = sin(state.mouse_angle) * state.power / 1000.0f * max_power;

		float xmax, ymax;
		trajectory::bounding_box(position.y, velo_x, velo_y, -state.gravity.y, &xmax, &ymax);

		path_builder trajectory_path(glm::vec2(0, position.y));

		float delta_x = xmax / 10;

		for (int i = 1 ; i <= 10; i++) {
			float x = i * delta_x;
			float height;
			trajectory::height_at(position.y, velo_x, velo_y, -state.gravity.y, x, &height);
			
			trajectory_path.line_to(glm::vec2(x, height));
		}

		trajectory_mesh.update(trajectory_path);

		matrix_builder model;
		model
			.translate(glm::vec3(position.x - state.terrain_left, 0, 0))
			.scale(glm::vec3(1, 1, 1));

		render_one(trajectory_mesh, shape_outline_shader, projection, model.matrix, 1.0f/*, screen_width, screen_height*/);
	}

	void render_ground() {
		float terrain_x = 0;

		line_mesh.bind();
		for (auto& ground : state.terrain.ground) {

			for (auto& segment : ground.segments) {
				segment.y1, segment.y2;
				glm::vec3 scale = glm::vec3(1, (segment.y2 - segment.y1), 1);
				glm::vec3 translate = glm::vec3(terrain_x, segment.y1, 0);
				
				matrix_builder model;
				model
					.translate(translate)
					.scale(scale);

				line_material.bind(0, state.terrain_width / 2, glm::vec3(0.8f, 0.7f, 0.5f), glm::vec3(0.2f, 0.2f, 0.1f), projection, model.matrix);
				line_mesh.render();
			}
			terrain_x += 1;
		}
		line_mesh.unbind();
	}

	void render_tank(scorch_player& player) {
		glm::vec3 player_translate(player.position.x - state.terrain_left, player.position.y, 0);

		matrix_builder barrel_model;
		barrel_model
			.translate(player_translate)
			.rotateZ(player.direction)
			;

		turret_shape.matrix = glm::scale(glm::mat4(1), glm::vec3(state.barrel_radius, state.barrel_radius, 1));
		turret_shape.render(projection, barrel_model.matrix);

		matrix_builder body_model;
		body_model
			.translate(player_translate)
			;

		body_shape.matrix = glm::scale(glm::mat4(1), glm::vec3(state.tank_radius, state.tank_radius, 1));
		body_shape.render(projection, body_model.matrix);

		matrix_builder health_model;
		health_model
			.translate(player_translate)
			.translate(glm::vec3(-8, state.tank_radius * 2.0f, 0))
			;


		float health = player.health / (float)player.max_health;
		glm::vec3 health_color;
		if (health < 0.2) {
			health_color = glm::vec3(1.0f, 0.2f, 0.2f);
		} if (health < 0.6) {
			health_color = glm::vec3(1.0f, 1.0f, 0.2f);
		} else {
			health_color = glm::vec3(0.2f, 1.0f, 0.2f);
		}

		health_bar.render(projection, health_model.matrix, health_color, health);
		// health bar = shape med dynamisk scalet/fargelagt quad inni?; vil ha border


	}

	void render_bullet(scorch_bullet& bullet) {
		// bullet radius at the frame
		auto radius_range = bullet.end_radius - bullet.start_radius;
		auto radius_unit = 1;// used with diggers only: bullet.frame / (float)bullet.target_frames;
		auto bullet_radius = (int)(bullet.start_radius + radius_unit  * radius_range);

		glm::vec3 scale(bullet_radius, bullet_radius, 1);
		glm::vec3 translate(bullet.position.x - state.terrain_left, bullet.position.y, 0);
		
		matrix_builder bullet_model;
		bullet_model
			.translate(translate)
			.scale(scale);

		bullet_shape.render(projection, bullet_model.matrix);
	}

	void render_explosion(scorch_explosion& explosion) {
		auto radius_range = explosion.target_radius;
		auto radius_unit = explosion.frame / (float)explosion.target_frames;
		auto explosion_radius = (int)(radius_unit  * radius_range);
		glm::vec3 scale(explosion_radius);
		glm::vec3 translate(explosion.position.x - state.terrain_left, explosion.position.y, 0);

		matrix_builder explosion_model;
		explosion_model
			.translate(translate)
			.scale(scale);

		explosion_shape.render(projection, explosion_model.matrix);
	}


	void render_bonus(scorch_bonus& bonus) {
		float bonus_radius = 5;

		glm::vec3 translate(bonus.position.x - state.terrain_left, bonus.position.y, 0);

		glm::mat4 model = glm::translate(glm::mat4(1), translate);

		bonus_shape.matrix = glm::scale(glm::mat4(1), glm::vec3(bonus_radius));
		bonus_shape.polygon_material.material_color = glm::vec3(1, 1, 0.5f);
		bonus_shape.thickness = 0.75f;
		bonus_shape.render(projection, model);
	}

};
