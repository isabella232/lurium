#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <algorithm>
#include <memory>
#include <cassert>
#include <time.h>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include "lurium/math/quadtree.h"
#include "lurium/math/diffing_quadtree.h"
#include "lurium/host/main_loop.h"
#include "lurium/gl/shader.h"
#include "lurium/gl/shader_plain.h"
#include "lurium/gl/texture.h"
#include "lurium/gl/primitives.h"
#include "lurium/gl/render_texture.h"
#include "lurium/gl/graphics.h"
#include "lurium/host/data_serializer.h"
#include "lurium/host/websocket.h"
#include "picojson.h"
#include "lurium/host/httprequest.h"

#include "lurium/math/point.h"
#include "lurium/math/random.h"
#include "../scorch_server/ground.h"
#include "../scorch_server/scorch.h"
#include "../scorch_server/scorch_message.h"

#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "cabin_ttf.h"
#include "logo_png.h"

#include "lurium/host/control_frontend_client.h"
#include "lurium/host/welcome_view.h"
#include "lurium/host/welcome_controller.h"
#include "scorch_view.h"
#include "scorch_controller.h"

material_light material_light::temp_nolight(false, false, false);

struct scorch_ui_state : main_loop_state {
	bool is_playing;
	scorch_state scorch;
	welcome_state welcome;
	scorch_ui_state() : is_playing(false) {}
};

struct scorch_ui_main : main_loop {
	scorch_ui_state& state;
	bitmap_font normal_font;
	bitmap logo;
	scorch_controller game;
	welcome_controller welcome;

	scorch_ui_main(scorch_ui_state& viewstate)
		: main_loop(viewstate)
		, state(viewstate)
		, normal_font(cabin_ttf, cabin_ttf_size)
		, logo(logo_png, logo_png_size)
		, game(viewstate.scorch, this, normal_font)
		, welcome(viewstate.welcome, this, "scorch", normal_font, logo)
	{
		state.scorch.game_connected = std::bind(&scorch_ui_main::game_connected, this);
		state.scorch.game_disconnected = std::bind(&scorch_ui_main::game_disconnected, this);
		state.scorch.game_welcome = std::bind(&scorch_ui_main::game_welcome, this);
		state.scorch.game_joined = std::bind(&scorch_ui_main::game_joined, this);
		state.scorch.game_dead = std::bind(&scorch_ui_main::game_dead, this);
		state.scorch.game_xp_changed = std::bind(&scorch_ui_main::game_xp_changed, this);
		state.scorch.game_weapons_changed = std::bind(&scorch_ui_main::game_weapons_changed, this);

		welcome.server_selected = std::bind(&scorch_ui_main::server_selected, this, std::placeholders::_1);
		welcome.server_changing = std::bind(&scorch_ui_main::server_changing, this);
		welcome.server_join = std::bind(&scorch_ui_main::server_join, this, std::placeholders::_1);

		if (state.is_playing) {
			// på java har vi ikke environmenten enda
			welcome.hide();
		}
	}

	~scorch_ui_main() {
		state.scorch.game_connected = nullptr;
		state.scorch.game_disconnected = nullptr;
		state.scorch.game_welcome = nullptr;
		state.scorch.game_joined = nullptr;
		state.scorch.game_dead = nullptr;
		state.scorch.game_xp_changed = nullptr;
		state.scorch.game_weapons_changed = nullptr;
	}
	
	void game_welcome() {
		game.weapon_buttons.update_weapons();

	}

	void game_weapons_changed() {
		game.game_weapons_changed();
	}

	void game_xp_changed() {
		game.game_xp_changed();
	}

	void server_selected(const std::string& url) {
		state.scorch.connect(url);
	}

	void server_changing() {
		state.scorch.disconnect();
	}

	void server_join(const std::string& nick) {
		state.scorch.join(nick);
	}

	void game_connected() {
		welcome.socket_connected();
	}

	void game_disconnected() {
		state.is_playing = false;
		welcome.socket_disconnected();
		welcome.show();
	}

	void game_joined() {
		printf("onjoined\n");// << std::endl;
		state.is_playing = true;
		welcome.hide();
	}

	void game_dead() {
		printf("ded\n");// << std::endl;
		state.is_playing = false;
		welcome.show();
	}

	void render() {
		welcome.poll();
		state.scorch.poll();

		if (state.is_playing) {
			state.scorch.update_server();
		}

		main_loop::render();

		glFlush();
	}

	virtual void resize(const window_event_data& e) {
		game.resize(e);
		welcome.resize(e);
	}

};


#if !defined(__ANDROID__)

int main(int argc, char* argv[]) {
	srand(time(NULL));

	scorch_ui_state state;
	{ scorch_ui_main loop(state); loop.run(); }

	return 0;
}

#else

extern "C" main_loop_state* lurium_create_state() {
	return new scorch_ui_state();
}

extern "C" void lurium_destroy_state(main_loop_state* state) {
	assert(state != NULL);
	delete state;
}

extern "C" main_loop* lurium_create_main_loop(main_loop_state* state_ptr) {
	assert(state_ptr != NULL);
	scorch_ui_state* state = (scorch_ui_state*)state_ptr;
	return new scorch_ui_main(*state);
}

extern "C" void lurium_destroy_main_loop(main_loop* loop) {
	delete loop;
}

#endif