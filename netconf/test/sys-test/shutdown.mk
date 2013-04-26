# ----------------------------------------------------------------------------|
# Shutdown tests
SHUTDOWN_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_SYSTEM)/shutdown-tests.cpp \
	                                                 shutdown.cpp \

ALL_SOURCES += $(SHUTDOWN_TEST_SUITE_SOURCES) 

ALL_SHUTDOWN_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(SHUTDOWN_TEST_SUITE_SOURCES)						

test-shutdown: $(call ALL_OBJECTS,$(ALL_SHUTDOWN_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-shutdown
