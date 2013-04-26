# ----------------------------------------------------------------------------|
# Simple Edit tests
EDIT_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/default-none-tests.cpp \
                           simple-edit-running.cpp \

ALL_SOURCES += $(EDIT_TEST_SUITE_SOURCES) 

ALL_EDIT_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(EDIT_TEST_SUITE_SOURCES)						

test-simple-edit-running: $(call ALL_OBJECTS,$(ALL_EDIT_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-simple-edit-running
