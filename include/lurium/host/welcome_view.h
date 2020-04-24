#pragma once



struct overlay_context : ui_context {
	overlay_quad overlay;
	bool dirty;

	overlay_context(ui_context* parent, int width, int height) 
		: ui_context(parent)
		, overlay(width, height) 
	{
		dirty = true;
	}

	virtual void redraw() = 0;

	void request_redraw() {
		dirty = true;
	}

	void render() {
		if (dirty) {
			redraw();
			dirty = false;
		}
		if (!visible) {
			return ;
		}
		overlay.render();
	}

};

struct button : ui_element {
	overlay_context& context;
	//bool visible;
	rect<int> position;
	std::string label;
	bool checked;
	bool pressed;
	bool pressing;
	pixel face_color, face_checked_color, face_pressed_color;
	int index;
	std::function<void(button&)> clicked_callback;
	std::function<void(button&)> lbuttondown_callback;
	std::function<void(button&)> lbuttonup_callback;

	button(overlay_context& parent, int _index = -1) : context(parent) {
		context.elements.push_back(this);
		visible = true;
		checked = false;
		pressed = false;
		pressing = false;
		index = _index;
		face_color = { 140, 140, 140, 255 };
		face_checked_color = { 255, 255, 255, 255 };
		face_pressed_color = { 192, 192, 192, 255 };
	}

	~button() {
		auto i = std::find(context.elements.begin(), context.elements.end(), this);
		assert(i != context.elements.end());
		context.elements.erase(i);
	}

	bool hit_button(int x, int y) const {
		if (!visible) {
			return false;
		}
		return position.hit(x, y);
	}

	void update_button(graphics& g) const {
		if (!visible) {
			return;
		}

		auto& p = position;

		if (pressed) {
			g.fill_color = face_pressed_color;
		} else
		if (checked) {
			g.fill_color = face_checked_color;
		}
		else {
			g.fill_color = face_color;
		}

		g.fill_rectangle(p.left, p.top, p.right - p.left, p.bottom - p.top);

		auto size = g.measure_string(label, 2);

		auto center_x = (p.right - p.left) / 2 - size.x / 2;
		auto center_y = (p.bottom - p.top) / 2 - size.y / 2;

		g.draw_string_border(p.left + center_x, p.top + center_y, label, 2, { 0, 0, 0, 255 });
	}

	bool lbuttondown(const mouse_event_data& e) {

		auto p = context.overlay.unproject(glm::vec2(e.x, e.y));
		int mouse_x = (int)p.x;
		int mouse_y = (int)p.y;

		if (hit_button(mouse_x, mouse_y)) {
			pressed = true;
			pressing = true;
			context.get_root()->set_capture(e.id, this);
			context.request_redraw();
			if (lbuttondown_callback) lbuttondown_callback(*this);
			return true;
		}
		return false;
	}

	bool mousemove(const mouse_event_data& e) {
		// pressed = true, request redraw

		if (pressing) {
			auto p = context.overlay.unproject(glm::vec2(e.x, e.y));
			int mouse_x = (int)p.x;
			int mouse_y = (int)p.y;

			bool hit = hit_button(mouse_x, mouse_y);
			if (hit != pressed) {
				pressed = hit;
				context.request_redraw();
			}

		}
		return false;
	}

	bool lbuttonup(const mouse_event_data& e) {
		if (pressing) {
			pressing = false;
			context.get_root()->set_capture(e.id, NULL);
			if (lbuttonup_callback) lbuttonup_callback(*this);

			if (pressed) {
				pressed = false;
				context.request_redraw();
				if (clicked_callback) clicked_callback(*this);
				return true;
			}
		}
		return false;
	}


	virtual bool keydown(const keyboard_event_data& e) {
		return false;
	}

	virtual bool keyup(const keyboard_event_data& e) {
		return false;
	}

	virtual void render() {
	}
};

struct welcome_state {
	int selected_mode;
	std::vector<std::string> modes;
	bool start_button_visible;

	welcome_state() 
		: selected_mode(-1)
		, start_button_visible(false)
	{
	}
};

struct welcome_view : overlay_context {
	welcome_state& state;
	bitmap_font& normal_font;
	const bitmap& logo;
	int logo_width, logo_height;

	glm::mat4x4 projection;
	int window_width;
	int window_height;

	std::shared_ptr<button> start_button;
	std::vector<std::shared_ptr<button>> mode_buttons;

	std::function<void(void)> start_clicked_callback;
	std::function<void(int)> mode_clicked_callback;

	welcome_view(welcome_state& viewstate, ui_context* parent, bitmap_font& font, const bitmap& logobitmap)
		: overlay_context(parent, 256, 512)
		, state(viewstate)
		, normal_font(font)
		, logo(logobitmap)
		//, overlay(256, 512)
	{

		float box = 0.5f;
		projection = glm::ortho<float>(-box, box, -box, box, 0, 10);
		start_button = std::make_shared<button>(*this);
		//start_button->visible = false; redraw sets this
		start_button->label = "Play";
		start_button->position = rect<int>::from_dimensions(10, 256 + 30, overlay.hud_bitmap.width - 20, 50);
		start_button->clicked_callback = std::bind(&welcome_view::start_button_clicked, this, std::placeholders::_1);

		if (logo.width > overlay.hud_bitmap.width) {
			auto ratio = logo.height / (float)logo.width;
			logo_width = overlay.hud_bitmap.width;
			logo_height = overlay.hud_bitmap.width * ratio;
		} else {
			logo_width = logo.width;
			logo_height = logo.height;
		}

		//update_texture();
	}

	void update_modes() {
		// name, index
		// create button bar or dropdown, or ui somethign; callbacker pr item, or as param, or view global ?
		mode_buttons.clear();
		int index = 0;
		for (const auto& mode : state.modes) {
			auto b = std::make_shared<button>(*this, index);
			b->label = mode;
			b->checked = (state.selected_mode == index);
			b->position = rect<int>::from_dimensions(0, logo_height + 30 + index * 35, overlay.hud_bitmap.width, 30);
			b->clicked_callback = std::bind(&welcome_view::mode_button_clicked, this, std::placeholders::_1);
			b->index = index;
			mode_buttons.push_back(b);
			index++;
		}

		request_redraw();
	}

	void mode_button_clicked(button& b) {
		if (mode_clicked_callback) mode_clicked_callback(b.index);
	}

	void start_button_clicked(button& b) {
		if (start_clicked_callback) start_clicked_callback();
	}

	void redraw() {
		graphics g(overlay.hud_bitmap);
		g.fill_color = { 128, 128, 128, 128 };
		g.fill_rectangle(0, 0, g.screen.width, g.screen.height);

		g.draw_bitmap(0, 0, logo_width, logo_height, &logo, 0, 0, logo.width, logo.height);

		g.font = &normal_font;
		g.font_size = 20;
		g.color = { 255, 255, 255, 255 };

		g.draw_string_border(0, logo_height, "Game modes:", 2, { 0, 0, 0, 255 });

		//int buttons_per_row = mode_buttons.size() /;
		for (const auto& mode_button : mode_buttons) {
			mode_button->update_button(g);
		}

		g.font_size = 48;
		start_button->visible = state.start_button_visible;
		start_button->update_button(g);

		overlay.hud_texture.update();
	}

	void resize(const window_event_data& e) {
		window_width = e.width;
		window_height = e.height;

		// TODO: if overlay still fits in x2 (or x4), then do that
		// overlay works by showing pixel-perfect size, 

		auto width_scale = window_width / (float)overlay.hud_bitmap.width;
		auto height_scale = window_height / (float)overlay.hud_bitmap.height;

		auto scale = std::max(width_scale, height_scale);
		overlay.scale = std::max(1.0f, scale);

		overlay.resize(e.width, e.height, overlay_quad::position::center, overlay_quad::position::center);


		auto tl = (overlay.project_to_world(projection, glm::vec2(10, 256)) + 1.0f / 2.0f) * glm::vec2(window_width, window_height);
		auto br = (overlay.project_to_world(projection, glm::vec2(overlay.hud_bitmap.width - 10, 256 + 24)) + 1.0f / 2.0f) * glm::vec2(window_width, window_height);
		auto tw = br.x - tl.x;
		auto th = tl.y - br.y;
		parent_context->get_root()->set_welcome_textbox_position(tl.x, tl.y, tw, th);
	}
};
