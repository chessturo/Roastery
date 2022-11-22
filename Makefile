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
	green ::= $(shell echo "\033[0;92m")
	green ::= $(strip $(green))
	red ::= $(shell echo "\033[0;31m")
	red ::= $(strip $(red))
	reset ::= $(shell echo "\033[0m")
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
clean: testclean
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

# =============================================================================
# Testing Variables
GTEST_BUILD_DIR ::= vendor/googletest/build
GTEST_LIB_DIR ::= $(GTEST_BUILD_DIR)/lib
GTEST_LIB ::= $(GTEST_LIB_DIR)/libgtest.a
GMOCK_LIB ::= $(GTEST_LIB_DIR)/libgmock.a
GMOCK_MAIN_LIB ::= $(GTEST_LIB_DIR)/libgmock_main.a
GTEST ::= gtest
GMOCK ::= gmock
GMOCK_MAIN ::= gmock_main
GTEST_INCLUDE ::= vendor/googletest/googletest/include/
GMOCK_INCLUDE ::= vendor/googletest/googlemock/include/

TEST_BUILD_DIR ::= $(BUILD_DIR)/test
TEST_OBJ_DIR ::= $(TEST_BUILD_DIR)/obj/$(BUILD)
TEST_DEP_DIR ::= $(TEST_BUILD_DIR)/dep

TEST_DIR ::= test
TEST_SRC_DIR ::= $(TEST_DIR)/src
TEST_SRCS ::= $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_OBJS ::= $(foreach SRC,$(TEST_SRCS),$(TEST_OBJ_DIR)/$(notdir $(SRC:.cpp=.o)))
TEST_DEPS ::= $(foreach SRC,$(TEST_SRCS),$(TEST_DEP_DIR)/$(notdir $(SRC:.cpp=.d)))

MOCK_DIR ::= $(TEST_DIR)/mock

TEST_EXE ::= $(TEST_DIR)/test-$(BUILD)

DEPFLAGS.test = -MT $@ -MMD -MP -MF $(TEST_DEP_DIR)/$*.d
CFLAGS.test ::= $(CFLAGS) -I$(GTEST_INCLUDE) -I$(GMOCK_INCLUDE) -I$(MOCK_DIR) -L$(GTEST_LIB_DIR)
TEST_LIBS ::= -l$(GMOCK_MAIN) -l$(GMOCK) -l$(GTEST)

.PHONY: test testclean
test: $(TEST_EXE)
	@./$(TEST_EXE)
testclean:
	rm -rf test/test-*
	@# Uncomment for `testclean` to force a recompile of gtest
	@#rm -rf $(GTEST_BUILD_DIR)

# -----------------------------------------------------------------------------
# Build objects
$(TEST_EXE): $(GTEST_LIB) $(GTEST_MAIN_LIB) $(GMOCK_LIB) $(filter-out $(OBJ_DIR)/roast.o,$(OBJS)) $(TEST_OBJS) $(MOCK_OBJS)
	$(info Linking $(green)$@$(reset) due to $?)
	@$(CXX) $(DEPFLAGS.test) $(CFLAGS.test) -o $@ $(filter-out $(OBJ_DIR)/roast.o,$(OBJS)) $(TEST_OBJS) $(MOCK_OBJS) $(TEST_LIBS)

$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp | $(TEST_DEP_DIR)/%.d $(TEST_OBJ_DIR) $(TEST_DEP_DIR)
	$(info Building $(green)$@$(reset) due to $?)
	@$(CXX) $(DEPFLAGS.test) $(CFLAGS.test) -c -o $@ $< $(TEST_LIBS)

$(GTEST_LIB) $(GMOCK_LIB) $(GMOCK_MAIN_LIB):
	$(info Building $(green)Googletest library$(reset))
	@cd vendor/googletest && mkdir -p build && cd build && cmake .. > /dev/null && $(MAKE) > /dev/null

# -----------------------------------------------------------------------------
# Help make deal with test deps
$(TEST_DEPS):
-include $(TEST_DEPS)

# =============================================================================
# Order only targets for directory structure
$(TEST_OBJ_DIR):
	@mkdir -p $(TEST_OBJ_DIR)
$(TEST_DEP_DIR):
	@mkdir -p $(TEST_DEP_DIR)

