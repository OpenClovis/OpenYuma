# ----------------------------------------------------------------------------|
# Test Harness Support Sources
YUMA_COMMON_TEST_SUPPORT_SOURCES := \
       $(YUMA_TEST_ROOT)/support/checkers/checker-group.cpp \
       $(YUMA_TEST_ROOT)/support/checkers/string-presence-checkers.cpp \
       $(YUMA_TEST_ROOT)/support/checkers/log-entry-presence-checkers.cpp \
       $(YUMA_TEST_ROOT)/support/callbacks/sil-callback-log.cpp \
       $(YUMA_TEST_ROOT)/support/callbacks/callback-checker.cpp \
       $(YUMA_TEST_ROOT)/support/callbacks/sil-callback-controller.cpp \
       $(YUMA_TEST_ROOT)/support/db-models/device-test-db.cpp \
       $(YUMA_TEST_ROOT)/support/db-models/device-test-db-debug.cpp \
       $(YUMA_TEST_ROOT)/support/db-models/state-data-test-db.cpp \
       $(YUMA_TEST_ROOT)/support/nc-session/abstract-nc-session.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/nc-base-query-test-engine.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/nc-default-operation-config.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/nc-query-test-engine.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/nc-query-utils.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/nc-strings.cpp \
       $(YUMA_TEST_ROOT)/support/nc-query-util/yuma-op-policies.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/abstract-global-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/test-context.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/base-suite-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/query-suite-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/simple-container-module-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/simple-yang-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/device-module-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/device-get-module-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/device-module-common-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/state-get-module-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/fixtures/state-data-module-common-fixture.cpp \
       $(YUMA_TEST_ROOT)/support/misc-util/cpp-unit-op-formatter.cpp \
       $(YUMA_TEST_ROOT)/support/misc-util/log-utils.cpp \
       $(YUMA_TEST_ROOT)/support/misc-util/ptree-utils.cpp \
       $(YUMA_TEST_ROOT)/support/misc-util/base64.cpp \
       $(YUMA_TEST_ROOT)/support/msg-util/NCMessageBuilder.cpp \
       $(YUMA_TEST_ROOT)/support/msg-util/xpo-query-builder.cpp \
       $(YUMA_TEST_ROOT)/support/msg-util/state-data-query-builder.cpp \

