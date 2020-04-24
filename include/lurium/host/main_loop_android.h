#if defined(__ANDROID__)

#include <jni.h>

#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "lurium", __VA_ARGS__);

#include <cassert>
#include <string>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct main_loop_state {
	JavaVM* java_vm; // use vm to get env for thread
	jobject java_loop;
};

struct main_loop : main_loop_base {

	// The Android main loop bits are implemented in Java in the com.anders_e.lurium project under lurium_android
	// The Java side uses the JNI C API in main_loop_android_jni.cpp to call back into the main loop implementation.
	main_loop_state& main_state;
	std::string welcome_text;

	main_loop(main_loop_state& state) : main_state(state) {
		// need java env in state, because app constructor can call f.ex textbox apis
	}

	~main_loop() {
	}

	int run() {
		// unused
		return 0;
	}

	virtual void set_welcome_textbox_visibility(bool visible) {

		JNIEnv* java_env;
		main_state.java_vm->AttachCurrentThread(&java_env, NULL);

		// call into java
		jclass mainLoopClass = java_env->GetObjectClass(main_state.java_loop);
		assert(mainLoopClass != NULL);

		// (Z)V == takes a bool, returns void
		jmethodID methodID = java_env->GetMethodID(mainLoopClass, "SetWelcomeTextboxVisibility", "(Z)V");
		assert(methodID != NULL);
		java_env->CallVoidMethod(main_state.java_loop, methodID, visible);
	}

	virtual void set_welcome_textbox_position(int x, int y, int width, int height) {

		JNIEnv* java_env;
		main_state.java_vm->AttachCurrentThread(&java_env, NULL);

		jclass mainLoopClass = java_env->GetObjectClass(main_state.java_loop);
		assert(mainLoopClass != NULL);

		// (IIII)V == takes four ints, returns void
		jmethodID methodID = java_env->GetMethodID(mainLoopClass, "SetWelcomeTextboxPosition", "(IIII)V");
		assert(methodID != NULL);
		java_env->CallVoidMethod(main_state.java_loop, methodID, x, y, width, height);
	}

	virtual void get_welcome_text(std::string& text) {
		text = welcome_text;
	}
};

#endif