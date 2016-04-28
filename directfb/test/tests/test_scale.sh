#!/bin/bash

#========================================================

echo "TESTING: dfbtest_scale..."

#========================================================

TESTS_DIR=`dirname $0`
source ${TESTS_DIR}/test_common.inc

TEST_IMAGE=${ISM_DIR}/test/img/dfb_logo-alpha.png
${DFB_BIN_DIR}/dfbtest_scale ${TEST_IMAGE} $@

#========================================================

echo ""
echo "DONE: Check ${SCREENSHOT_DIR} for output"

#========================================================

