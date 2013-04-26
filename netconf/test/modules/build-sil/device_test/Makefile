YUMA_ROOT=../../../../
include $(YUMA_ROOT)/test/make-rules/common.mk

# ----------------------------------------------------------------------------|
# Test Harness Support Sources
BASE_NAME := device_test

ifeq ($(DISABLE_TEST_CB_LOGGING),)
YUMA_TEST_SOURCES := $(BASE_NAME).cpp
else
YUMA_TEST_SOURCES := $(BASE_NAME).c
endif

ALL_SOURCES := $(YUMA_TEST_SOURCES)

TARGET_LIB := lib$(BASE_NAME).so 

$(TARGET_LIB): output/$(BASE_NAME).o 
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -rdynamic -Wl,-soname,$@ -o $@ $^
	mkdir -p ../../sil
	cp $(TARGET_LIB) ../../sil

TARGETS := $(TARGET_LIB)

# Ensure that libboost_test is not included in the SIL build
LIBS := $(filter-out boost_unit_test_framework, $(LIBS))

include $(YUMA_ROOT)/test/make-rules/common-rules.mk

