# ----------------------------------------------------------------------------|
# Startup tests
STARTUP_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests-startup.cpp \
				                        simple-edit-startup-false.cpp \

ALL_SOURCES += $(STARTUP_TEST_SUITE_SOURCES) 

ALL_STARTUP_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(STARTUP_TEST_SUITE_SOURCES)						

test-simple-edit-startup-false: $(call ALL_OBJECTS,$(ALL_STARTUP_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-simple-edit-startup-false
