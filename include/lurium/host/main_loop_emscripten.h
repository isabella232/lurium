
/*
Info lifted from:
https://github.com/kripken/emscripten/blob/master/tests/webgl_create_context2.cpp
https://github.com/kripken/emscripten/blob/master/tests/webgl_destroy_context.cpp
https://github.com/kripken/emscripten/blob/master/tests/test_browser.py

NOTE: the JS main loop expects DOM state in the loader:
	<canvas id="canvas"></canvas>
	<input id="welcomeTextbox" type="text"></input>
	<script>
	var Module = {
		canvas:canvas,
		welcomeTextbox:welcomeTextbox
	};
	</script>
	<script src=program.js></script>
*/
#ifdef __EMSCRIPTEN__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

// the JS side:
extern "C" void welcome_textbox_set_visibility(int visible);
extern "C" void welcome_textbox_set_position(int x, int y, int width, int height);
extern "C" int welcome_textbox_get_text(char* buffer, int length);


struct main_loop_state {
};

struct main_loop : main_loop_base {

	main_loop(main_loop_state&) {
		register_events();

		EmscriptenWebGLContextAttributes attrs;
		attrs.premultipliedAlpha = false;
		emscripten_webgl_init_context_attributes(&attrs);

		EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
		context = emscripten_webgl_create_context(0, &attrs);

		emscripten_webgl_make_context_current(context);
		// this comes automatically with legacy gl emulation defined? .. also shader needs to enable?, TODO: alert if not exist
		emscripten_webgl_enable_extension(context, "GL_OES_standard_derivatives");
	}

	int run() {

		double client_w, client_h, xscale, yscale;
		emscripten_get_element_css_size(NULL, &client_w, &client_h);
		printf("resise to %f %f\n", client_w, client_h);
		emscripten_set_canvas_size(client_w, client_h);
		glViewport(0, 0, client_w, client_h);

		update_size(client_w, client_h);

		// emscripten_webgl_destroy_context(context);

		// no code runs after this line
		emscripten_set_main_loop_arg(main_loop_one, this, 0, 1);
		//emscripten_set_main_loop_arg(main_loop_one, this, 60, 1);
		printf("ini don22!\n");

		return 0;
	}

	void register_events() {
		emscripten_set_mousemove_callback("#canvas", this, 0, Emscripten_HandleMouseEvent);
		emscripten_set_mousedown_callback("#canvas", this, 0, Emscripten_HandleMouseEvent);
		emscripten_set_mouseup_callback("#canvas", this, 0, Emscripten_HandleMouseEvent);

		emscripten_set_keydown_callback("#window", this, 0, Emscripten_HandleKey);
		emscripten_set_keyup_callback("#window", this, 0, Emscripten_HandleKey);
		//emscripten_set_keypress_callback("#window", data, 0, Emscripten_HandleKeyPress);

		emscripten_set_resize_callback("#window", this, 0, Emscripten_HandleResize);
	}

	static EM_BOOL Emscripten_HandleKey(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
		main_loop* self = (main_loop*)userData;
		keyboard_event_data e;
		e.char_code = keyEvent->keyCode;
		switch (eventType) {
		case EMSCRIPTEN_EVENT_KEYDOWN:
			self->keydown(e);
			break;
		case EMSCRIPTEN_EVENT_KEYUP:
			self->keyup(e);
			break;
		}
	}

	static EM_BOOL Emscripten_HandleMouseEvent(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
		main_loop* self = (main_loop*)userData;
		int mx = mouseEvent->canvasX;
		int my = mouseEvent->canvasY;
		
		double client_w, client_h;
		emscripten_get_element_css_size(NULL, &client_w, &client_h);

		mouse_event_data e;
		e.id = 0;
		e.x = mx / client_w * 2 - 1;
		e.y = my / client_h * 2 - 1;
		e.left_button = mouseEvent->button == 0;
		e.right_button = mouseEvent->button == 2;

		switch (eventType) {
			case EMSCRIPTEN_EVENT_MOUSEMOVE:
				self->mousemove(e);
				break;
			case EMSCRIPTEN_EVENT_MOUSEDOWN:
				if (e.left_button) {
					self->lbuttondown(e);
				}/* else if (e.right_button) {
				 self->rbuttondown(e);
				 }*/
				break;
			case EMSCRIPTEN_EVENT_MOUSEUP:
				if (e.left_button) {
					self->lbuttonup(e);
				}/* else if (e.right_button) {
				 self->rbuttondown(e);
				 }*/
				break;
		}
		return 0;
	}

	void update_size(int client_w, int client_h) {
		window_event_data e;
		e.width = client_w;
		e.height = client_h;
		resize(e);
	}
	
	static EM_BOOL Emscripten_HandleResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
		main_loop* self = (main_loop*)userData;

		double client_w, client_h, xscale, yscale;
		emscripten_get_element_css_size(NULL, &client_w, &client_h);
		emscripten_set_canvas_size(client_w, client_h);
		glViewport(0, 0, client_w, client_h);
		self->update_size(client_w, client_h);
		return 0;
	}

	// The "main loop" function.
	static void main_loop_one(void* arg) {
		//printf("main_loop\n");
		main_loop* self = (main_loop*)arg;
		self->render();
	}

	virtual void set_welcome_textbox_visibility(bool visible) {
		welcome_textbox_set_visibility(visible);
	}

	virtual void set_welcome_textbox_position(int x, int y, int width, int height) {
		welcome_textbox_set_position(x, y, width, height);
	}

	virtual void get_welcome_text(std::string& text) {
		char pc[64] = {};
		int bytes = welcome_textbox_get_text(pc, 64);
		text = std::string(pc, bytes);
	}
};

#endif
