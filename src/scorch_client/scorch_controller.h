#pragma once

// todo: kan vi arve noe slik at vi får welcome?
// current context hierarchy:
// mainloop
//   scorchcontroller
//     scorchview
//     weapons
//     arrow buttons
//   welcomecontroller 
//     welcomeview

struct scorch_controller : ui_context {
	scorch_state& state;
	scorch_view game;
	weapon_view weapon_buttons;
	arrow_buttons_view arrow_buttons;

	scorch_controller(scorch_state& viewstate, ui_context* parent, bitmap_font& font)
		: ui_context(parent)
		, state(viewstate)
		, game(viewstate, this, font)
		, weapon_buttons(viewstate, this, game.projection, font, 1024, 128, game.shape_fill_shader, game.shape_outline_shader)
		, arrow_buttons(this, font)
	{
		weapon_buttons.weapon_button_clicked_callback = std::bind(&scorch_controller::weapon_button_clicked, this, std::placeholders::_1);
		weapon_buttons.weapon_upgrade_button_clicked_callback = std::bind(&scorch_controller::weapon_upgrade_button_clicked, this, std::placeholders::_1);

		// if reattaching
		weapon_buttons.update_weapons();

	}


	void game_weapons_changed() {

		for (auto& w : weapon_buttons.weapon_upgrade_buttons) {
			w->visible = state.player_can_upgrade;
		}
		weapon_buttons.request_redraw();

	}

	void game_xp_changed() {
		weapon_buttons.request_redraw();
	}

	void weapon_button_clicked(button& b) {
		//b.index; or userdata or something typed?
		// TODO: subscrube to state weapon events ???, flip button checked there
		weapon_buttons.weapon_buttons[state.player_weapon]->checked = false;
		state.player_weapon = (weapon_type)b.index;
		weapon_buttons.weapon_buttons[state.player_weapon]->checked = true;
		weapon_buttons.request_redraw();
	}

	void weapon_upgrade_button_clicked(button& b) {
		auto upgrade_weapon = (weapon_type)b.index;
		state.weapon_upgrades.push(upgrade_weapon);
	}

	bool keydown(const keyboard_event_data& e) {
		int i;
		switch (e.char_code) {
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			i = e.char_code - '0';
			if (i <= state.player_weapons.size()) {
				weapon_button_clicked(*weapon_buttons.weapon_buttons[i - 1]);
				//weapon_buttons[weapon].checked = false;
				//weapon = (weapon_type)(i - 1);
				//weapon_buttons[weapon].checked = true;
				//dirty_hud = true;
			}
			break;
		case ' ':
			state.space_pressed = true;
			break;
		case 'a':
		case 'A':
			state.a_pressed = true;
			break;
		case 'd':
		case 'D':
			state.d_pressed = true;
			break;
		case 'l':
		case 'L':
			// queue upgarde on current weapon
			state.weapon_upgrades.push(state.player_weapon);
			break;
		}
		return false;
	}

	bool keyup(const keyboard_event_data& e) {
		switch (e.char_code) {
		case ' ':
			state.space_pressed = false;
			break;
		case 'a':
		case 'A':
			state.a_pressed = false;
			break;
		case 'd':
		case 'D':
			state.d_pressed = false;
			break;
		case 'l':
		case 'L':
			break;
		}
		return false;
	}


	virtual void resize(const window_event_data& e) {
		// TODO: move to ui_context
		game.resize(e);
		weapon_buttons.overlay.resize(e.width, e.height, overlay_quad::position::last, overlay_quad::position::first);
		arrow_buttons.resize(e.width, e.height);

	}

};