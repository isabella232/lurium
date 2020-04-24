#NDK_TOOLCHAIN_VERSION := 4.8
APP_OPTIM := debug    # Build the target in debug mode. 
#APP_ABI := x86 # armeabi-v7a # Define the target architecture to be ARM.
APP_STL := c++_static # beast uses to_string missing from gnustl, glm uses features in gnustl missing from stlport
APP_CPPFLAGS := -DPICOJSON_USE_LOCALE=0 -std=c++11 -frtti -fexceptions    # This is the place you enable exception.
APP_PLATFORM := android-9 # Define the target Android version of the native application.
