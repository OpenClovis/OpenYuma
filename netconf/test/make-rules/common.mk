# ----------------------------------------------------------------------------|
# Set directory variables
YUMA_SRC_ROOT=$(YUMA_ROOT)/src
YUMA_TEST_ROOT=$(YUMA_ROOT)/test
YUMA_STUBS_ROOT=$(YUMA_TEST_ROOT)/stubs
YUMA_SYS_TEST_ROOT=$(YUMA_ROOT)/test/sys-test
YUMA_TEST_SUITE_BASE=$(YUMA_TEST_ROOT)/test-suites
YUMA_TEST_SUITE_COMMON=$(YUMA_TEST_SUITE_BASE)/common
YUMA_TEST_SUITE_INTEG=$(YUMA_TEST_SUITE_BASE)/integ
YUMA_TEST_SUITE_SYSTEM=$(YUMA_TEST_SUITE_BASE)/system

INCLUDES := ./ \
            $(YUMA_SRC_ROOT)/agt \
            $(YUMA_SRC_ROOT)/ncx \
            $(YUMA_SRC_ROOT)/platform \
            $(YUMA_ROOT) \
            $(DESTDIR)/usr/include/libxml2 \
            $(DESTDIR)/usr/include/libxml2/libxml \
            $(DESTDIR)/include/libxml2 \
            $(DESTDIR)/include/libxml2/libxml 

LIBS := xml2 \
        boost_unit_test_framework

ALL_SOURCES_OUTPUT = $(addprefix output/,$(notdir $(1)))
ALL_OBJECTS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(call ALL_SOURCES_OUTPUT,$(1))))

define MAKE_TEST
g++ $^ $(LDFLAGS) -o $@
endef

