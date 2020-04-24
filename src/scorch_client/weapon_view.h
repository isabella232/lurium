#pragma once

struct weapon_view : overlay_context {
	bitmap_font& normal_font;
	scorch_state& state;
	glm::mat4& projection;
	progress_shape weapon_bar;

	std::vector<std::shared_ptr<button>> weapon_buttons;
	std::vector<std::shared_ptr<button>> weapon_upgrade_buttons;
	std::function<void(button&)> weapon_button_clicked_callback;
	std::function<void(button&)> weapon_upgrade_button_clicked_callback;

	weapon_view(scorch_state& viewstate, ui_context* parent, glm::mat4& _projection, bitmap_font& font, int width, int height, const shader_plain& shape_fill_shader, const path_line_shader& shape_outline_shader)
		: overlay_context(parent, width, height)
		, state(viewstate)
		, projection(_projection)
		, normal_font(font)
		, weapon_bar(shape_fill_shader, shape_outline_shader, 140, 10, 3, 1.0f)
	{

	}

	void update_weapons(/*const std::vector<std::string> weapon_names*/) {
		// we have 512x512 of texture, render buttons right aligned
		int index = 0;

		int button_width = 150;
		int button_margin = 10;
		int left = overlay.hud_bitmap.width - state.weapon_names.size() * button_width;

		weapon_buttons.clear();
		weapon_upgrade_buttons.clear();
		//state.player_weapons.clear();

		for (const auto& weapon_name : state.weapon_names) {
			auto b = std::make_shared<button>(*this, index);
			b->label = weapon_name;
			b->position = rect<int>::from_dimensions(left + index * button_width, 10, button_width - button_margin, 50);
			b->checked = ((int)state.player_weapon == index);
			b->clicked_callback = weapon_button_clicked_callback;
			weapon_buttons.push_back(b);

			auto upgrade = std::make_shared<button>(*this, index);
			upgrade->label = "Upgrade";
			upgrade->visible = state.player_can_upgrade;
			upgrade->position = rect<int>::from_dimensions(left + index * button_width, 10 + 50, button_width - button_margin, 25);
			upgrade->clicked_callback = weapon_upgrade_button_clicked_callback;
			weapon_upgrade_buttons.push_back(upgrade);
			index++;
		}

		request_redraw();
	}

	void redraw() {
		graphics g(overlay.hud_bitmap);
		g.fill_color = { 0, 0, 0, 0 };
		g.fill_rectangle(0, 0, g.screen.width, g.screen.height);
		g.font = &normal_font;
		g.font_size = 25;
		g.color = { 255, 255, 255, 255 };

		//g.draw_string(10, 10, "ANDERS", 0);

		for (const auto& b : weapon_buttons) {
			b->update_button(g);
		}

		for (const auto& b : weapon_upgrade_buttons) {
			b->update_button(g);
		}

		char pc[128];

		g.font_size = 20;
		int index = 0;
		for (const auto& b : weapon_buttons) {
			sprintf(pc, "%i)", index + 1);
			g.draw_string_border(b->position.left, b->position.top, pc, 1, { 0, 0, 0, 0 });

			if (!state.player_weapons.empty()) {
				sprintf(pc, "Level %i", state.player_weapons[index].level + 1);
				auto w = g.measure_string(pc, 1);
				g.draw_string_border(b->position.right - w.x, b->position.top, pc, 1, { 0, 0, 0, 0 });
			}
			index++;
		}

		g.font_size = 48;
		sprintf(pc, "XP: %i", state.player_xp);
		auto measure = g.measure_string(pc, 2);
		g.draw_string_border(g.screen.width - measure.x - 5, 80, pc, 2, { 0, 0, 0, 0 });
		overlay.hud_texture.update();
	}

	void render() {
		overlay_context::render();

		if (weapon_buttons.empty()) return;
		if (weapon_buttons.size() != state.player_weapons.size()) return;

		int index = 0;
		for (const auto& b : weapon_buttons) {

			// button dims in texture space -> flip texture coords -> project to screen space -> unproject to world space
			auto wep = state.player_weapons[index];
			float cooldown = wep.cooldown / (float)wep.max_cooldown;

			// the buttons are painted on a texture, which is projected on a quad which positions and scales itself to stay at some left/right/center/top/middle/bottom
			// the weapon regen bar is rendered in world coordinates, because the outline border is based on world coords
			// originally rendered weapon bar in texture space, which had simpler math, but caused the outline border to scale inappropriately
			// thickness could be given in pixel or world pos: using world means borders scale with main projection

			glm::vec2 button_position(b->position.left, b->position.bottom - weapon_bar.height);
			auto reworld_topleft = overlay.project_to_world(projection, button_position);
			auto reworld_bottomright = overlay.project_to_world(projection, button_position + glm::vec2(weapon_bar.width, weapon_bar.height));
			auto reworld_size = reworld_bottomright - reworld_topleft;

			matrix_builder model;
			model
				.translate(glm::vec3(reworld_topleft, 0.0f))
				.scale(glm::vec3(reworld_size.x / weapon_bar.width, reworld_size.y / weapon_bar.height, 0.0f))
				;
			weapon_bar.health_shape.thickness = 0.5f;
			weapon_bar.render(projection, model.matrix, glm::vec3(0.4f, 0.4f, 0.4f), cooldown);
			index++;
		}


	}
};
