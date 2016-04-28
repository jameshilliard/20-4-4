#!/bin/bash

#========================================================

echo "TESTING: dfbtest_blit..."

#========================================================

TESTS_DIR=`dirname $0`
source ${TESTS_DIR}/test_common.inc

TEST_IMAGE=${ISM_DIR}/test/img/dfb_logo-alpha.png
${DFB_BIN_DIR}/dfbtest_blit -b ${TEST_IMAGE} $@

#========================================================

echo ""
echo "DONE"

#========================================================

