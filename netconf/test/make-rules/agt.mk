include $(YUMA_TEST_ROOT)/make-rules/agt-sources.mk

# ----------------------------------------------------------------------------|
# Stub Source Files
AGT_STUB_SOURCES := $(YUMA_STUBS_ROOT)/agt/agt_ncxserver.cpp

ALL_SOURCES += $(AGT_SOURCES) $(AGT_STUB_SOURCES)
