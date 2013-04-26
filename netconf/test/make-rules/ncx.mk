include $(YUMA_TEST_ROOT)/make-rules/ncx-sources.mk
 
# ----------------------------------------------------------------------------|
# Stub Source Files
NCX_STUB_SOURCES := $(YUMA_STUBS_ROOT)/ncx/send_buff.cpp

ALL_SOURCES += $(NCX_SOURCES) $(NCX_STUB_SOURCES)

# ----------------------------------------------------------------------------|
# Set dependencies on curversion.h
output/ncx.o: curversion.h

# ----------------------------------------------------------------------------|
# Generate include file defining the SVN version
curversion.h:
	echo "#define SVNVERSION \"`svnversion`\"" > curversion.h

