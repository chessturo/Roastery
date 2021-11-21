# Roastery main makefile

# Copyright 2021 Mitchell Levy

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# =============================================================================
# Set up

# -----------------------------------------------------------------------------
# Build settings

# To build a debug version of Roastery, run make as such:
# `make [target] BUILD=debug`
# This Makefile will build a release version of Roastery by default

OK_BUILDS ::= debug release

ifndef BUILD
  BUILD ::= release
else
  ifeq "$(filter $(BUILD),$(OK_BUILDS))" ""
    $(error Invalid BUILD "$(BUILD)". Valid options are: $(OK_BUILDS))
  endif
endif

ifndef NOCOLOR
	green ::= $(shell echo -e "\033[0;92m")
	green ::= $(strip $(green))
	red ::= $(shell echo -e "\033[0;31m")
	red ::= $(strip $(red))
	reset ::= $(shell echo -e "\033[0m")
	reset ::= $(strip $(reset))
else
	green ::=
	red ::=
	reset ::=
endif

# -----------------------------------------------------------------------------
# Common variables

# Set up all the necessary directory/file variables
BUILD_DIR ::= build
OBJ_DIR ::= $(BUILD_DIR)/obj/$(BUILD)
DEP_DIR ::= $(BUILD_DIR)/dep
SRC_DIR ::= src
LIB_DIR ::= lib
SRCS ::= $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(LIB_DIR)/*.cpp)
OBJS ::= $(foreach SRC,$(SRCS),$(OBJ_DIR)/$(notdir $(SRC:.cpp=.o)))
DEPS ::= $(foreach SRC,$(SRCS),$(DEP_DIR)/$(notdir $(SRC:.cpp=.d)))

# Keep track of executables generated
EXES ::= roast

# Build programs
CXX ::= g++
LD ::= ld

# -----------------------------------------------------------------------------
# Flags

# Set flags for $(CC), based on the value of $(BUILD)
CFLAGS.base ::= -Wall -Wextra -pthread -std=c++17 -I./$(SRC_DIR)/include -I./$(LIB_DIR)/include
CFLAGS.debug ::= -g
CFLAGS.release ::= -O3
CFLAGS ::= $(CFLAGS.$(BUILD)) $(CFLAGS.base)

# Set flags for generating dependencies, with the GCC options as default
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

# =============================================================================
# Recipies to build

# -----------------------------------------------------------------------------
# Phony targets
.PHONY: all clean doc
all: roast
clean:
	rm -rf $(BUILD_DIR) $(EXES)
doc:
	$(info Generating docs...)
	@doxygen > /dev/null

# -----------------------------------------------------------------------------
# Targets that build executables
roast: $(OBJS)
	@$(CXX) $(CFLAGS) -o $@ $^
	$(info Linking $(green)$@$(reset) due to $?)

# -----------------------------------------------------------------------------
# Help `make` deal properly with generated dependencies
$(DEPS):
-include $(DEPS)

# -----------------------------------------------------------------------------
# Generate object files

# Delete the implicit .cpp -> .o rule
%.o: %.cpp
# Targets that build intermediates
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(DEP_DIR)/%.d $(OBJ_DIR) $(DEP_DIR)
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<
	$(info Building $(green)$@$(reset) due to $?)
$(OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp | $(DEP_DIR)/%.d $(OBJ_DIR) $(DEP_DIR)
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<
	$(info Building $(green)$@$(reset) due to $?)

# =============================================================================
# Order-only targets so we have the needed directory structure
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
$(DEP_DIR):
	@mkdir -p $(DEP_DIR)
