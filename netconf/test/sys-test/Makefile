YUMA_ROOT=../..
include $(YUMA_ROOT)/test/make-rules/common.mk

# ----------------------------------------------------------------------------|
# Common Sources for all tests
ALL_SOURCES := 
TARGETS := 
include $(YUMA_TEST_ROOT)/support/support.mk
include $(YUMA_TEST_ROOT)/support/sys-support.mk

BASE_SOURCES := $(ALL_SOURCES)

# ----------------------------------------------------------------------------|
# Include makefiles for test executables
include lock-load-running.mk
include lock-load-candidate.mk
include simple-edit-candidate.mk
include simple-edit-running.mk
include device-edit-candidate.mk
include device-edit-running.mk
include shutdown.mk

# ----------------------------------------------------------------------------|
# Add additional libraries specific to system testing
LIBS += ssh2

# ----------------------------------------------------------------------------|
include $(YUMA_TEST_ROOT)/make-rules/common-rules.mk
