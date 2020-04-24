#if defined(__ANDROID__)
#include <cassert>
#include <vector>

#include "lurium/host/main_loop.h"

extern "C" JNIEXPORT jlong JNICALL Java_com_anders_1e_lurium_MainLoop_InternalCreateState(JNIEnv *env, jobject self) {
	// NOTE/TODO: factory is android specific, could pass in vm and loop and let state deal with refs
	main_loop_state* state = lurium_create_state();
	env->GetJavaVM(&state->java_vm);
	state->java_loop = env->NewGlobalRef(self);
	return (jlong)state;
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalDestroyState(JNIEnv *env, jobject self, jlong state_ptr) {
	main_loop_state* state = (main_loop_state*)state_ptr;
	env->DeleteGlobalRef(state->java_loop);
	lurium_destroy_state(state);
}

extern "C" JNIEXPORT jlong JNICALL Java_com_anders_1e_lurium_MainLoop_InternalCreateMainLoop(JNIEnv *env, jobject self, jlong state_ptr) {
	main_loop_state* state = (main_loop_state*)state_ptr;
	main_loop* loop = lurium_create_main_loop(state);
	return (jlong)loop;
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalDestroyMainLoop(JNIEnv *env, jobject /* this */, jlong loop_ptr) {
	main_loop* loop = (main_loop*)loop_ptr;
	lurium_destroy_main_loop(loop);
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalRender(JNIEnv *env, jobject /* this */, jlong loop_ptr) {
	main_loop* loop = (main_loop*)loop_ptr;
	loop->render();
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalResize(JNIEnv *env, jobject /* this */, jlong loop_ptr, jint width, jint height) {
	main_loop* loop = (main_loop*)loop_ptr;
	window_event_data e;
	e.width = width;
	e.height = height;
	loop->resize(e);
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalLbuttonDown(JNIEnv *env, jobject /* this */, jlong loop_ptr, jint id, jfloat x, jfloat y) {
	main_loop* loop = (main_loop*)loop_ptr;
	mouse_event_data e;
	e.id = id;
	e.x = x;
	e.y = y;
	e.left_button = true;
	e.right_button = false;
	loop->lbuttondown(e);
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalLbuttonUp(JNIEnv *env, jobject /* this */, jlong loop_ptr, jint id, jfloat x, jfloat y) {
	main_loop* loop = (main_loop*)loop_ptr;
	mouse_event_data e;
	e.id = id;
	e.x = x;
	e.y = y;
	e.left_button = true;
	e.right_button = false;
	loop->lbuttonup(e);
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalMouseMove(JNIEnv *env, jobject /* this */, jlong loop_ptr, jint id, jfloat x, jfloat y) {
	main_loop* loop = (main_loop*)loop_ptr;
	mouse_event_data e;
	e.id = id;
	e.x = x;
	e.y = y;
	e.left_button = false;
	e.right_button = false;
	loop->mousemove(e);
}

extern "C" JNIEXPORT void JNICALL Java_com_anders_1e_lurium_MainLoop_InternalSetWelcomeText(JNIEnv *env, jobject /* this */, jlong loop_ptr, jbyteArray text) {
	main_loop* loop = (main_loop*)loop_ptr;

	// expect byte array of utf-8 chars
	jbyte* text_input = env->GetByteArrayElements(text, NULL);
	jsize size = env->GetArrayLength(text);
	loop->welcome_text = std::string(text_input, text_input + size);
	env->ReleaseByteArrayElements(text, text_input, NULL);
}

#endif
