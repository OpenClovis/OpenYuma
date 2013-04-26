# ----------------------------------------------------------------------------|
# Discard changes tests
DISCARD_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/discard-changes-tests.cpp \
	                                                discard-changes.cpp \

ALL_SOURCES += $(DISCARD_TEST_SUITE_SOURCES) 

ALL_DISCARD_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(DISCARD_TEST_SUITE_SOURCES)						

test-discard-changes: $(call ALL_OBJECTS,$(ALL_DISCARD_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-discard-changes
