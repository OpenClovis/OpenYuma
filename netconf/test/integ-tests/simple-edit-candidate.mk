# ----------------------------------------------------------------------------|
# Simple Edit tests
EDIT_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests-candidate.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/simple-edit-tests-validate.cpp \
						         $(YUMA_TEST_SUITE_COMMON)/discard-changes-tests.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/default-none-tests.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/callback-failures-tests.cpp \
						   simple-edit-candidate.cpp \

ALL_SOURCES += $(EDIT_TEST_SUITE_SOURCES) 

ALL_EDIT_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(EDIT_TEST_SUITE_SOURCES)						

test-simple-edit-candidate: $(call ALL_OBJECTS,$(ALL_EDIT_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-simple-edit-candidate
