/*
	Header-only GL context for Windows and the browser
*/

#include <functional>
#include <algorithm>
#include <vector>
#include <map>

struct window_event_data {
	int width, height;
};

struct mouse_event_data {
	int id; // which of multiple cursors
	float x, y; // scaled to -1..1
	bool left_button;
	bool right_button;
};

struct keyboard_event_data {
	int char_code;
};

struct ui_element {
	bool visible;

	ui_element() : visible(true) {
	}
	virtual ~ui_element() {}
	virtual bool lbuttondown(const mouse_event_data& e) = 0;
	virtual bool mousemove(const mouse_event_data& e) = 0;
	virtual bool lbuttonup(const mouse_event_data& e) = 0;
	virtual bool keydown(const keyboard_event_data& e) = 0;
	virtual bool keyup(const keyboard_event_data& e) = 0;
	virtual void render() = 0;
};

struct main_loop_base;

// ui_context = toplevel element; vi trenger base ui_element for global capture
// main loop_base kan iplemtere denne?
struct ui_context : ui_element {
	ui_context* parent_context;
	std::vector<ui_element*> elements;

	ui_context(ui_context* parent) : parent_context(parent) {
		if (parent_context != NULL) {
			parent_context->elements.push_back(this);
		}
	}

	virtual ~ui_context() {
		if (parent_context != NULL) {
			auto i = std::find(parent_context->elements.begin(), parent_context->elements.end(), this);
			assert(i != parent_context->elements.end());
			parent_context->elements.erase(i);
		}
	}

	virtual main_loop_base* get_root() {
		// overide in actual root
		assert(parent_context != NULL);
		return parent_context->get_root();
	}

	virtual void render() {
		if (!visible) {
			return;
		}

		for (auto& element : elements) {
			element->render();
		}
	}

	bool lbuttondown(const mouse_event_data& e) {
		// NOTE events i motsatt rekkefølge som rendering? render på topp = håndterer events først
		if (!visible) {
			return false;
		}

		for (auto i = elements.rbegin(); i != elements.rend(); ++i) {
			auto& element = *i;
			if (element->lbuttondown(e))
				return true;
		}
		return false;
	}

	bool mousemove(const mouse_event_data& e) {
		if (!visible) {
			return false;
		}

		for (auto i = elements.rbegin(); i != elements.rend(); ++i) {
			auto& element = *i;
			if (element->mousemove(e))
				return true;
		}
		return false;
	}

	bool lbuttonup(const mouse_event_data& e) {
		if (!visible) {
			return false;
		}

		for (auto i = elements.rbegin(); i != elements.rend(); ++i) {
			auto& element = *i;
			if (element->lbuttonup(e))
				return true;
		}
		return false;
	}

	virtual bool keydown(const keyboard_event_data& e) {
		if (!visible) {
			return false;
		}

		for (auto i = elements.rbegin(); i != elements.rend(); ++i) {
			auto& element = *i;
			if (element->keydown(e))
				return true;
		}
		return false;
	}

	virtual bool keyup(const keyboard_event_data& e) {
		if (!visible) {
			return false;
		}

		for (auto i = elements.rbegin(); i != elements.rend(); ++i) {
			auto& element = *i;
			if (element->keyup(e))
				return true;
		}
		return false;
	}
};

/*
struct group_context : ui_context {
	// TODO: merge with ui_contet, allow null parent_context for roots; or we have thad "odd recurrign template parameter" to "inject" root and group contexts

	// kan vi istedet bare ha capture på root; its a top level concept

	ui_context* parent_context;

	group_context(ui_context* parent) : parent_context(parent) {
		assert(parent_context != NULL);
		parent_context->elements.push_back(this);
	}

	~group_context() {
		auto i = std::find(parent_context->elements.begin(), parent_context->elements.end(), this);
		assert(i != parent_context->elements.end());
		parent_context->elements.erase(i);
	}

	virtual main_loop_base* get_root() {
		return parent_context->get_root();
	}

	virtual ui_element* get_capture(int id) {
		return parent_context->get_capture(id);
	}

	virtual void set_capture(int id, ui_element* element) {
		assert(element == NULL || get_capture(id) == NULL);
		parent_context->set_capture(id, element);
	}
};*/

struct main_loop_base : ui_context {
	// TODO: one capture for each cursor .. up to three? four?
	//ui_element* capture;
	std::map<int, ui_element*> capture;

	main_loop_base() : ui_context(NULL) {
	}

	virtual ~main_loop_base() {}

	virtual main_loop_base* get_root() {
		return this;
	}

	virtual ui_element* get_capture(int id) {
		auto it = capture.find(id);
		if (it == capture.end()) {
			return NULL;
		}
		return it->second;
		//return capture;
	}

	virtual void set_capture(int id, ui_element* element) {
		if (element == NULL) {
			capture.erase(id);
		} else {
			capture[id] = element;
		}
	}

	bool lbuttondown(const mouse_event_data& e) {
		auto capture = get_capture(e.id);
		if (capture != NULL) {
			return capture->lbuttondown(e);
		}
		else {
			return ui_context::lbuttondown(e);
		}
	}

	bool mousemove(const mouse_event_data& e) {
		auto capture = get_capture(e.id);
		if (capture != NULL) {
			return capture->mousemove(e);
		}
		else {
			return ui_context::mousemove(e);
		}
	}

	bool lbuttonup(const mouse_event_data& e) {
		auto capture = get_capture(e.id);
		if (capture != NULL) {
			return capture->lbuttonup(e);
		}
		else {
			return ui_context::lbuttonup(e);
		}
	}

	virtual void resize(const window_event_data&) = 0;

	virtual void set_welcome_textbox_visibility(bool visible) = 0;
	virtual void set_welcome_textbox_position(int x, int y, int width, int height) = 0;
	virtual void get_welcome_text(std::string& text) = 0;
};



#ifdef __EMSCRIPTEN__
#include "main_loop_emscripten.h"
#elif defined(_WIN32)
#include "main_loop_win32.h"
#elif defined(__ANDROID__)
#include "main_loop_android.h"

// Application must implement global lurium_create_main_loop() factory function on Android
extern "C" main_loop_state* lurium_create_state();
extern "C" void lurium_destroy_state(main_loop_state* state_ptr);

extern "C" main_loop* lurium_create_main_loop(main_loop_state* state_ptr);
extern "C" void lurium_destroy_main_loop(main_loop*);

#else
#error "This project supports only Windows, Android and Emscripten"
#endif
