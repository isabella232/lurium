/*
native os textbox, ie android EditText w/popup keyboard, or html <input>, or win32 textbox, etc

android's lurium host inserts a textbox in the ui thread, and deals with communication between gl thread
*/
/*
#include "lurium/host/main_loop.h"

void welcome_textbox_set_visibility(main_loop* loop, int visible) {
	// call into java!
	// TOOD: move to main_loop_android.h
	loop->java_env;
	loop->java_loop;
}

void welcome_textbox_set_size(main_loop* loop, int x, int y, int width, int height) {
	// call into java!
	loop->java_env;
	loop->java_loop;
}
*/