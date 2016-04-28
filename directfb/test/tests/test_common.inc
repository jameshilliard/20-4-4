
#========================================================

if [ -z "${SRCROOT}" ]; then
    echo "ERROR: SRCROOT is not set."
    exit 1
fi

if [ -z "${TIVO_SYSTEM}" ]; then
    echo "ERROR: TIVO_SYSTEM is not set."
    exit 1
fi

DFB_VERSION=DirectFB-1.4.5

ISM_DIR=${SRCROOT}/opensource/directfb
ISM_OBJDIR=${ISM_DIR}/obj/${TIVO_SYSTEM}/${DFB_VERSION}
DFB_BIN_DIR=${ISM_OBJDIR}/install/bin

# set/override screenshot-dir
SCREENSHOT_DIR=/tmp/dfb-test-out
mkdir -p ${SCREENSHOT_DIR}

# clean up old files
rm -f /tmp/dfb-test-out/*

DFBARG_SCREENSHOT_DIR=screenshot-dir=${SCREENSHOT_DIR}
if [ -z "${DFBARGS}" ]; then
    export DFBARGS="${DFBARG_SCREENSHOT_DIR}"
else
    export DFBARGS="${DFBARGS},${DFBARG_SCREENSHOT_DIR}"
fi

# TiVo-added 'feature'
# Skip the surface dump alpha channel => .pgm image output
export DFB_DUMP_OMIT_ALPHA=1

#========================================================

