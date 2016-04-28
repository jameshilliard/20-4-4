
# This file is licensed under GNU General Public license.                      
#                                                                              
# This file is free software: you can redistribute and/or modify it under the  
# terms of the GNU General Public License, Version 2, as published by the Free 
# Software Foundation.                                                         
#                                                                             
# This program is distributed in the hope that it will be useful, but AS-IS and
# WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, 
# except as permitted by the GNU General Public License is prohibited.         
#                                                                              
# You should have received a copy of the GNU General Public License, Version 2 
# along with this file; if not, see <http://www.gnu.org/licenses/>.            

##############################################################################
# Build the E1000 module in a LINUX way
##############################################################################
#

ENTROPIC_E1000_C_FILES +=                        \
  ../Common/Src/hostos_stubs.c                   \
  ../Common/Src/misc_driver_stubs.c              \

ENTROPIC_E1000_H_FILES +=                        \

ENTROPIC_E1000_INC_DIRS +=                       \

ENTROPIC_E1000_EXTRA_CFLAGS +=                   \

TARGET = e1000-osnone
OBJ    = $(patsubst %.c, %.o, $(ENTROPIC_E1000_C_FILES))
LIBS   =

CXXFLAGS += $(ENTROPIC_COMMON_CFLAGS)
CXXFLAGS += $(ENTROPIC_E1000_EXTRA_CFLAGS)
CXXFLAGS += $(patsubst %, -I $(CURDIR)/%, $(ENTROPIC_E1000_INC_DIRS))
CXXFLAGS += -DLICENSE_ENTROPIC_ONLY_STUBS

OBJ_LCL = $(notdir $(OBJ))

OUTPUT_FILES=$(OBJ_LCL) $(OBJ) $(TARGET)

.PHONY: default
default: start $(TARGET)

.PHONY: start
start:
	echo " OBJ_LCL=$(OBJ_LCL)"
	echo " OBJ=$(OBJ)"
	echo " CXXFLAGS=$(CXXFLAGS)"
	echo " clearing objects..."
	-rm -rf $(OUTPUT_FILES)

.PHONY: $(TARGET)
$(TARGET): $(OBJ) $(LIBS)
	@echo Creating $@ linked object...
	gcc -o $@.a $(OBJ_LCL)
	@echo Skipped.

.PHONY: %.o
%.o : 
	@echo Rule $@ Compiling for item $<...
	gcc -c $(CXXFLAGS) $(patsubst %.o, %.c, $@) -o $(notdir $@)
	@echo Rule $@ Compiled item $<

clean:
	-rm -f *.o *.a $(OUTPUT_FILES)

