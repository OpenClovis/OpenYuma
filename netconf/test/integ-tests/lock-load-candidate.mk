# ----------------------------------------------------------------------------|
# Candidate Tests 
CANDIDATE_DB_SOURCES := $(YUMA_TEST_SUITE_COMMON)/module-load-test-suite.cpp \
                        $(YUMA_TEST_SUITE_COMMON)/db-lock-test-suite-common.cpp \
                        $(YUMA_TEST_SUITE_COMMON)/db-lock-test-suite-candidate.cpp \
					    lock-load-candidate.cpp

ALL_SOURCES += $(CANDIDATE_DB_SOURCES) 

ALL_CANDIDATE_DB_SOURCES := $(BASE_SOURCES) $(CANDIDATE_DB_SOURCES)						

test-lock-load-candidate: $(call ALL_OBJECTS,$(ALL_CANDIDATE_DB_SOURCES)) | yuma-op
	$(MAKE_TEST)

TARGETS += test-lock-load-candidate
