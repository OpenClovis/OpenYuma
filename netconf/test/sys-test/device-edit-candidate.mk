# ----------------------------------------------------------------------------|
# Device Edit tests
EDIT_TEST_SUITE_SOURCES := $(YUMA_TEST_SUITE_COMMON)/device-tests-misc.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/device-tests-create.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/device-tests-merge.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/device-tests-replace.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/device-tests-get.cpp \
                           $(YUMA_TEST_SUITE_COMMON)/device-tests-delete.cpp \
                           device-edit-candidate.cpp \

ALL_SOURCES += $(EDIT_TEST_SUITE_SOURCES) 

ALL_EDIT_TEST_SUITE_SOURCES := $(BASE_SOURCES) $(EDIT_TEST_SUITE_SOURCES)						

test-device-edit-candidate: $(call ALL_OBJECTS,$(ALL_EDIT_TEST_SUITE_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-device-edit-candidate
