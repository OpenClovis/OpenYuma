# ----------------------------------------------------------------------------|
# Running Tests 
RUNNING_DB_SOURCES := $(YUMA_TEST_SUITE_COMMON)/module-load-test-suite.cpp \
                      $(YUMA_TEST_SUITE_COMMON)/db-lock-test-suite-common.cpp \
                      $(YUMA_TEST_SUITE_COMMON)/db-lock-test-suite-running.cpp \
					  lock-load-running.cpp

ALL_SOURCES += $(RUNNING_DB_SOURCES) 

ALL_RUNNING_DB_SOURCES := $(BASE_SOURCES) $(RUNNING_DB_SOURCES)						

test-lock-load-running: $(call ALL_OBJECTS,$(ALL_RUNNING_DB_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-lock-load-running
