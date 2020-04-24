LOCAL_PATH:= $(call my-dir) # Get the local path of the project.
include $(CLEAR_VARS) # Clear all the variables with a prefix "LOCAL_"

# need to reference an arbiutrary boost library in order to pull in the headers; not really in use
LOCAL_STATIC_LIBRARIES := boost_system
LOCAL_SRC_FILES:= ../main.cpp ../../lurium/websocket.cpp ../../lurium/httprequest.cpp ../../lurium/main_loop_android_jni.cpp
# ../../lurium/welcome_textbox_android.cpp
LOCAL_C_INCLUDES := ../../include
LOCAL_LDLIBS := -lEGL -lGLESv2 -llog
LOCAL_MODULE:= scorched # The name of the binary.

include $(BUILD_SHARED_LIBRARY) # Tell ndk-build that we want to build a shared library.

# boost 1_55_0 does not compile
$(call import-module,boost_1_63_0)



# TODO: GL ES linking her: https://developer.android.com/ndk/guides/stable_apis.html
