LOCAL_PATH:= $(call my-dir)

trace_cmd_SRC_FILES := \
	trace-cmd.c \
	trace-record.c \
	trace-read.c \
	trace-split.c \
	trace-listen.c \
	trace-stack.c \
	trace-options.c \
	parse-events.c \
	trace-seq.c \
	parse-filter.c \
	parse-utils.c \
	trace-util.c \
	trace-input.c \
	trace-ftrace.c \
	trace-output.c \
	trace-recorder.c \
	trace-restore.c \
	trace-usage.c \
	trace-blk-hack.c \
	glob.c \
	getline.c \
	splice.c \
#
unit_test_SRC_FILES := \
	glob.c \
	test_main.c \
	splice.c \
	getline.c \
#

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := trace-test
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_CFLAGS := -O2 -g
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	bionic/libc/kernel/arch-x86/

LOCAL_SRC_FILES := $(unit_test_SRC_FILES)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := trace-cmd
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_CFLAGS := -O2 -g
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	bionic/libc/kernel/arch-x86/

LOCAL_SRC_FILES := $(trace_cmd_SRC_FILES)

intermediates := $(call local-intermediates-dir)

GEN := $(intermediates)/tc_version.h

$(GEN) :
	echo "#define VERSION_CODE 258" > $@
	echo "#define EXTRAVERSION  0" >> $@
	echo "#define VERSION_STRING \"1.2.0\"" >> $@
	echo "#define FILE_VERSION 6" >> $@

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_C_INCLUDES += $(intermediates)/include

include $(BUILD_EXECUTABLE)

