# ----------------------------------------------------------------------------|
# Startup tests
STARTUP_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests-startup.cpp \
                              $(YUMA_TEST_SUITE_COMMON)/startup-delete-config-tests.cpp \
				                        simple-edit-startup-true.cpp \

ALL_SOURCES += $(STARTUP_TEST_SUITE_SOURCES) 

ALL_STARTUP_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(STARTUP_TEST_SUITE_SOURCES)						

test-simple-edit-startup-true: $(call ALL_OBJECTS,$(ALL_STARTUP_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-simple-edit-startup-true
