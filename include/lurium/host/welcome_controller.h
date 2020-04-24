#pragma once

struct welcome_controller : ui_context {
	//int selected_mode;
	welcome_state& state;
	welcome_view welcome;
	control_frontend_client control;

	std::function<void(const std::string&)> server_selected;
	std::function<void()> server_changing;
	std::function<void(const std::string&)> server_join;

	welcome_controller(welcome_state& viewstate, ui_context* parent, const std::string& protocol, bitmap_font& font, const bitmap& logo) 
		: ui_context(parent)
		, state(viewstate)
		, welcome(viewstate, this, font, logo) 
		, control(protocol)
	{

		welcome.start_clicked_callback = std::bind(&welcome_controller::start_clicked, this);
		welcome.mode_clicked_callback = std::bind(&welcome_controller::mode_clicked, this, std::placeholders::_1);

		if (state.selected_mode == -1) {
			// first time app startup
			control.load_modes();
		} else {
			welcome.update_modes();
		}
	}

	void hide() {
		welcome.visible = false;
		get_root()->set_welcome_textbox_visibility(false);
	}

	void show() {
		welcome.visible = true;
		get_root()->set_welcome_textbox_visibility(true);
	}

	void set_message(const std::string& text) {
		// f.ex was disconnected, is full ..?
	}

	void poll() {
		control.poll(
			std::bind(&welcome_controller::modes_loaded, this),
			std::bind(&welcome_controller::address_loaded, this, std::placeholders::_1, std::placeholders::_2)
		);
	}

	void resize(const window_event_data& e) {
		welcome.resize(e);
	}

	void modes_loaded() {
		state.modes.clear();
		for (auto m : control.modes) {
			char temp[32];
			sprintf(temp, "%i", m.players);
			auto mode_name = m.name;// +" (" + std::string(temp) + ")";
			state.modes.push_back(mode_name);
		}

		
		welcome.update_modes();

		if (state.selected_mode == -1 && !state.modes.empty()) {
			mode_clicked(0);
		}
	}

	void address_loaded(const std::string& address, const std::string& description) {
		// we have .. address and description
		if (server_selected) server_selected(address);
		//ws.open("ws://localhost:9002");
	}

	void mode_clicked(int index) {
		if (state.selected_mode >= 0) {
			welcome.mode_buttons[state.selected_mode]->checked = false;
		}

		state.selected_mode = index;
		welcome.mode_buttons[state.selected_mode]->checked = true;

		welcome.request_redraw();

		//ws.close();
		if (server_changing) server_changing();

		const auto& mode = state.modes[state.selected_mode];// control.modes[state.selected_mode].name;
		control.get_server_address(mode);
		// void address_loaded(string address) {ws.open(address)

		//ws.open("ws://localhost:9002");
	}

	void start_clicked() {
		std::string nick;
		get_root()->get_welcome_text(nick);
		printf("UTF NICK IS %s\n", nick.c_str());
		if (server_join) server_join(nick);
	}

	void socket_connected() {
		// enable start button
		//welcome.start_button->visible = true;
		state.start_button_visible = true;
		welcome.request_redraw();
	}

	void socket_disconnected() {
		// disable.start button
		//welcome.start_button->visible = false;
		state.start_button_visible = false;
		welcome.request_redraw();
	}


};

