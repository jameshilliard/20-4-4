#!/bin/bash

#========================================================

echo "TESTING: dfbtest_blit2..."

#========================================================

TESTS_DIR=`dirname $0`
source ${TESTS_DIR}/test_common.inc

${DFB_BIN_DIR}/dfbtest_blit2 $@

#========================================================

echo ""
echo "DONE: Check ${SCREENSHOT_DIR} for output"

#========================================================

