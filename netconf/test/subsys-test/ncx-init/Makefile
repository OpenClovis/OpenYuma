
YUMA_SRC_ROOT=../../../
YUMA_STUBS_ROOT=../../stubs/

INCLUDES = ./ \
           $(YUMA_SRC_ROOT)src/agt \
		   $(YUMA_SRC_ROOT)src/ncx \
		   $(YUMA_SRC_ROOT)src/platform \
		   /usr/include/libxml2 \
		   /usr/include/libxml2/libxml 

LIBS = xml2 \
       boost_unit_test_framework

DEBUG = -g
COMMON_FLAGS = -Wall $(DEBUG) $(addprefix -I,$(INCLUDES))

LDFLAGS = -Wall $(DEBUG) $(addprefix -l,$(LIBS))

CFLAGS = $(COMMON_FLAGS)
CXXFLAGS = $(COMMON_FLAGS)

# ----------------------------------------------------------------------------|
# netconf/src/ncx source files
NCX_SOURCES = $(YUMA_SRC_ROOT)src/ncx/b64.c \
              $(YUMA_SRC_ROOT)src/ncx/bobhash.c \
              $(YUMA_SRC_ROOT)src/ncx/cap.c \
              $(YUMA_SRC_ROOT)src/ncx/cfg.c \
              $(YUMA_SRC_ROOT)src/ncx/cli.c \
              $(YUMA_SRC_ROOT)src/ncx/conf.c \
              $(YUMA_SRC_ROOT)src/ncx/def_reg.c \
              $(YUMA_SRC_ROOT)src/ncx/dlq.c \
              $(YUMA_SRC_ROOT)src/ncx/ext.c \
              $(YUMA_SRC_ROOT)src/ncx/grp.c \
              $(YUMA_SRC_ROOT)src/ncx/help.c \
              $(YUMA_SRC_ROOT)src/ncx/log.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx_appinfo.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx_feature.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx_list.c \
              $(YUMA_SRC_ROOT)src/ncx/ncxmod.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx_num.c \
              $(YUMA_SRC_ROOT)src/ncx/ncx_str.c \
              $(YUMA_SRC_ROOT)src/ncx/obj.c \
              $(YUMA_SRC_ROOT)src/ncx/obj_help.c \
              $(YUMA_SRC_ROOT)src/ncx/op.c \
              $(YUMA_SRC_ROOT)src/ncx/plock.c \
              $(YUMA_SRC_ROOT)src/ncx/plock_cb.c \
              $(YUMA_SRC_ROOT)src/ncx/rpc.c \
              $(YUMA_SRC_ROOT)src/ncx/rpc_err.c \
              $(YUMA_SRC_ROOT)src/ncx/runstack.c \
              $(YUMA_SRC_ROOT)src/ncx/ses.c \
              $(YUMA_SRC_ROOT)src/ncx/ses_msg.c \
              $(YUMA_SRC_ROOT)src/ncx/status.c \
              $(YUMA_SRC_ROOT)src/ncx/tk.c \
              $(YUMA_SRC_ROOT)src/ncx/top.c \
              $(YUMA_SRC_ROOT)src/ncx/tstamp.c \
              $(YUMA_SRC_ROOT)src/ncx/typ.c \
              $(YUMA_SRC_ROOT)src/ncx/val.c \
              $(YUMA_SRC_ROOT)src/ncx/val_util.c \
              $(YUMA_SRC_ROOT)src/ncx/var.c \
              $(YUMA_SRC_ROOT)src/ncx/xml_msg.c \
              $(YUMA_SRC_ROOT)src/ncx/xmlns.c \
              $(YUMA_SRC_ROOT)src/ncx/xml_util.c \
              $(YUMA_SRC_ROOT)src/ncx/xml_val.c \
              $(YUMA_SRC_ROOT)src/ncx/xml_wr.c \
              $(YUMA_SRC_ROOT)src/ncx/xpath1.c \
              $(YUMA_SRC_ROOT)src/ncx/xpath.c \
              $(YUMA_SRC_ROOT)src/ncx/xpath_wr.c \
              $(YUMA_SRC_ROOT)src/ncx/xpath_yang.c \
              $(YUMA_SRC_ROOT)src/ncx/yang.c \
              $(YUMA_SRC_ROOT)src/ncx/yang_ext.c \
              $(YUMA_SRC_ROOT)src/ncx/yang_grp.c \
              $(YUMA_SRC_ROOT)src/ncx/yang_obj.c \
              $(YUMA_SRC_ROOT)src/ncx/yang_parse.c \
              $(YUMA_SRC_ROOT)src/ncx/yang_typ.c \
              $(YUMA_SRC_ROOT)src/ncx/yin.c \
              $(YUMA_SRC_ROOT)src/ncx/yinyang.c 

# ----------------------------------------------------------------------------|
# Stub Source Files
NCX_STUB_SOURCES := $(YUMA_STUBS_ROOT)ncx/send_buff.cpp

# ----------------------------------------------------------------------------|
# Test Harness Sources
YUMA_TEST_SOURCES := ncx-init-test.cpp \

ALL_SOURCES := $(YUMA_TEST_SOURCES) \
               $(NCX_STUB_SOURCES) \
               $(NCX_SOURCES) \

ALL_OBJECTS := $(addprefix output/,$(notdir $(ALL_SOURCES)))
ALL_OBJECTS := $(ALL_OBJECTS:.cpp=.o)
ALL_OBJECTS := $(ALL_OBJECTS:.c=.o)

ncx-init-test: $(ALL_OBJECTS)
	$(MAKE_TEST)

curversion.h:
	echo "#define SVNVERSION \"`svnversion`\"" > curversion.h

output/ncx.o: curversion.h

# ----------------------------------------------------------------------------|
# Generic make rules

TARGET := ncx-init-test

all: $(TARGET)

clean:
	rm -f $(TARGET) output/* curversion.h

define OBJECT_TEMPLATE

output/$(notdir $(basename $(1))).o: $(1) | output
	gcc $(CXXFLAGS) -c $$< -o $$@
endef

$(foreach S,$(ALL_SOURCES),$(eval $(call OBJECT_TEMPLATE,$(S))))

define MAKE_TEST
g++ $(LDFLAGS) $^ -o $@
endef

output:
	mkdir -p $@

