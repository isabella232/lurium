#pragma once

struct arrow_buttons_view : overlay_context {

	bitmap_font& normal_font;
	button right_button, left_button;
	button space_button;

	mouse_event_data arrow_button_event;
	bool arrow_button_down;

	arrow_buttons_view(ui_context* parent, bitmap_font& font)
		: overlay_context(parent, 128, 128)
		, normal_font(font)
		, right_button(*this)
		, left_button(*this)
		, space_button(*this)
		, arrow_button_down(false)
	{
		pixel color = { 128, 128, 128, 32 };
		// TODO: arrow bitsmaps? 
		left_button.position = rect<int>::from_dimensions(10, 10, 50, 50);
		left_button.face_color = color;
		left_button.label = "L";
		left_button.index = 'a'; // simulate 'a' keypress
		left_button.lbuttondown_callback = std::bind(&arrow_buttons_view::button_lbuttondown, this, std::placeholders::_1);
		left_button.lbuttonup_callback = std::bind(&arrow_buttons_view::button_lbuttonup, this, std::placeholders::_1);

		right_button.position = rect<int>::from_dimensions(70, 10, 50, 50);
		right_button.face_color = color;
		right_button.label = "R";
		right_button.index = 'd'; // simulate 'd' keypress
		right_button.lbuttondown_callback = std::bind(&arrow_buttons_view::button_lbuttondown, this, std::placeholders::_1);
		right_button.lbuttonup_callback = std::bind(&arrow_buttons_view::button_lbuttonup, this, std::placeholders::_1);

		space_button.position = rect<int>::from_dimensions(10, 70, 110, 50);
		space_button.face_color = color;
		space_button.label = "Fire";
		space_button.index = ' ';
		space_button.lbuttondown_callback = std::bind(&arrow_buttons_view::button_lbuttondown, this, std::placeholders::_1);
		space_button.lbuttonup_callback = std::bind(&arrow_buttons_view::button_lbuttonup, this, std::placeholders::_1);
	}

	void button_lbuttondown(button& b) {
		// can make index == keycode
		keyboard_event_data ke;
		ke.char_code = b.index;

		auto loop = parent_context->get_root();
		loop->keydown(ke);
	}

	void button_lbuttonup(button& b) {
		keyboard_event_data ke;
		ke.char_code = b.index;

		auto loop = parent_context->get_root();
		loop->keyup(ke);
	}

	void redraw() {
		graphics g(overlay.hud_bitmap);
		g.fill_color = { 0, 0, 0, 0 };
		g.fill_rectangle(0, 0, g.screen.width, g.screen.height);
		g.font = &normal_font;
		g.font_size = 25;
		g.color = { 255, 255, 255, 255 };

		right_button.update_button(g);
		left_button.update_button(g);
		space_button.update_button(g);

		overlay.hud_texture.update();
	}

	void resize(int screen_width, int screen_height) {

		// TODO: improve quad posititioning relative to top/left/center/middle/right/bottom, f.ex "100 px up from bottom right"
		// TODO: improve quad scaling, f.ex arrow buttons are really tiny on big resolution, and really big on small resolution

		// TODO: implicit rule, "buttons should cover quarter of screen estate"

		auto width_scale = screen_width / 4 / (float)overlay.hud_bitmap.width;
		auto height_scale = screen_height / 4 / (float)overlay.hud_bitmap.height;

		auto scale = std::min(width_scale, height_scale);
		overlay.scale = std::max(1.0f, scale);
		overlay.resize(screen_width, screen_height, overlay_quad::position::first, overlay_quad::position::last);
	}

};
