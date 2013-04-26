# ----------------------------------------------------------------------------|
# Set compiler flags
DEBUG = -g
COMMON_FLAGS = -fPIC -Wall $(DEBUG) $(addprefix -I,$(INCLUDES))

override LDFLAGS += -Wall $(DEBUG) $(addprefix -l,$(LIBS)) -Wl,--export-dynamic

override CFLAGS += $(COMMON_FLAGS)
override CXXFLAGS += $(COMMON_FLAGS) -std=c++0x

# ----------------------------------------------------------------------------|
# Generic make rules
.DEFAULT_GOAL := all
all: $(TARGETS)

clean:
	rm -rf $(TARGETS) output curversion.h

define OBJECT_C_TEMPLATE

output/$(notdir $(basename $(1))).o: $(1) | output
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

define OBJECT_CXX_TEMPLATE

output/$(notdir $(basename $(1))).o: $(1) | output
	$(CXX) $(CXXFLAGS) -c $$< -o $$@
endef

$(foreach S,$(filter %.c,$(ALL_SOURCES)),$(eval $(call OBJECT_C_TEMPLATE,$(S))))
$(foreach S,$(filter %.cpp,$(ALL_SOURCES)),$(eval $(call OBJECT_CXX_TEMPLATE,$(S))))

output:
	mkdir -p $@

yuma-op:
	mkdir -p $@
