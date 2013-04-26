# ----------------------------------------------------------------------------|
# Default none tests
NONE_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/default-none-tests.cpp \
                           default-none-running.cpp \

ALL_SOURCES += $(NONE_TEST_SUITE_SOURCES) 

ALL_NONE_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(NONE_TEST_SUITE_SOURCES)						

test-default-none-running: $(call ALL_OBJECTS,$(ALL_NONE_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-default-none-running
