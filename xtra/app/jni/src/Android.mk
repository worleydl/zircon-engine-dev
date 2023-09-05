LOCAL_PATH := $(call my-dir)

LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

# zirco\android\app\jni\src <-- @ src
TO_PROJECT_FOLDER := ../../../..

CORE_PATH_SDK_FOR_PROJECT  := ./xtra/SDK
#CORE_PATH_CORE_FOR_PROJECT := ./xtra/SDK/Core



CORE_PATH_SDK := $(TO_PROJECT_FOLDER)/$(CORE_PATH_SDK_FOR_PROJECT)

CORE_SDL_PATH := SDL2-devel-2.0.8-mingw/SDL2-2.0.8/i686-w64-mingw32/include

SDL_PATH := $(CORE_PATH_SDK)/$(CORE_SDL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# src here!
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(TO_PROJECT_FOLDER)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CORE_PATH_SDK)

# following line prints which is what the info command does.
$(info $(LOCAL_C_INCLUDES))



#MY_FILE_LIST  := $(wildcard $(LOCAL_PATH)/$(TO_PROJECT_FOLDER)/Mark_V/*.c)
#MY_FILE_LIST  += $(wildcard $(LOCAL_PATH)/$(CORE_PATH_CORE)/*.c)

MY_FILE_LIST  := $(wildcard $(LOCAL_PATH)/$(TO_PROJECT_FOLDER)/*.c)

LOCAL_SRC_FILES := $(MY_FILE_LIST:$(LOCAL_PATH)/%=%)

#LOCAL_CFLAGS    := -DCORE_PTHREADS -DCORE_SDL -DCORE_GL -DQUAKE_GAME -DGLQUAKE

LOCAL_CFLAGS    :=	-DCORE_SDL					\
					-DCORE_GL					\
					-DQUAKE_GAME				\
					-DGLQUAKE					\
					-DCONFIG_MENU				\
					-DCONFIG_CD					\
					-D_FILE_OFFSET_BITS=64		\
					-D__KERNEL_STRICT_NAMES		\
					-DUSE_RWOPS					\
					-DUSE_GLES2					\
					-DPLATFORM_ANDROID



#$(info $(LOCAL_PATH))
#$(info $(MY_FILE_LIST))
#$(error Bob)

LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

LOCAL_SHARED_LIBRARIES := SDL2

# libdl is the dynamic linking library.

#LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog mark v

LOCAL_LDLIBS := -lGLESv2 -lEGL -ldl -llog



LOCAL_SHORT_COMMANDS := true
APP_SHORT_COMMANDS := true

include $(BUILD_SHARED_LIBRARY)
