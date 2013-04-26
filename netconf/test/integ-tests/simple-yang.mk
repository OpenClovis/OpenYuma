# ----------------------------------------------------------------------------|
# Simple Yang Tests
YANG_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/simple-choice-tests.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/simple-must-tests.cpp \
                                                     simple-yang.cpp \

ALL_SOURCES += $(YANG_TEST_SUITE_SOURCES) 

ALL_YANG_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(YANG_TEST_SUITE_SOURCES)						

test-simple-yang: $(call ALL_OBJECTS,$(ALL_YANG_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-simple-yang
