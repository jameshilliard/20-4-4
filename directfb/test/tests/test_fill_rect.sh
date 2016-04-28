#!/bin/bash

#========================================================

echo "TESTING: dfbtest_fillrect..."

#========================================================

TESTS_DIR=`dirname $0`
source ${TESTS_DIR}/test_common.inc

${DFB_BIN_DIR}/dfbtest_fillrect $@

#========================================================

echo ""
echo "DONE: Check ${SCREENSHOT_DIR} for output"

#========================================================

