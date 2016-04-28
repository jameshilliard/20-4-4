###############################################################################
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

TARGET_DIR = obj/$(TIVO_SYSTEM)
SOURCE_DIR = $(TARGET_DIR)/wpa_supplicant-1.1
SOURCE_TAR = wpa_supplicant-1.1.tar.gz

PATCH ?= /usr/bin/patch
PATCH_FILES = $(wildcard patch/*.patch)

default : $(OPENSOURCE_CONFIG_STAMP)

include $(ISMRULES)

$(OPENSOURCE_UNTAR_STAMP) : $(SOURCE_TAR) $(PATCH_FILES)
	rm -rf $(TARGET_DIR)
	mkdir -p $(TARGET_DIR)
	tar -C $(TARGET_DIR) -xzvf $<
	mkdir -p $(STAMP_DIR)
	touch $@

$(OPENSOURCE_CONFIG_STAMP) : $(OPENSOURCE_RM_STAMP) $(OPENSOURCE_PATCH_STAMP)
	/usr/bin/env PATH="$(TOOLROOT)/bin:$(PATH)" $(MAKE) V=1 -C $(SOURCE_DIR)/wpa_supplicant all
	mkdir -p $(STAMP_DIR)
	touch $@

