YUMA_ROOT=../..
include $(YUMA_ROOT)/test/make-rules/common.mk

# ----------------------------------------------------------------------------|
# Common Sources for all tests
ALL_SOURCES := 
TARGETS := 
include $(YUMA_TEST_ROOT)/make-rules/ncx.mk
include $(YUMA_TEST_ROOT)/make-rules/agt.mk
include $(YUMA_TEST_ROOT)/support/support.mk
include $(YUMA_TEST_ROOT)/support/integ-support.mk
include $(YUMA_TEST_ROOT)/support/sys-support.mk

TARGETS := libyumatest.a

# ----------------------------------------------------------------------------|
libyumatest.a: $(call ALL_OBJECTS,$(ALL_SOURCES))
	ar rcs $@ $^ 

# ----------------------------------------------------------------------------|
include $(YUMA_TEST_ROOT)/make-rules/common-rules.mk
