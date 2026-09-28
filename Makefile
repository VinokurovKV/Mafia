CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
RUN_ARGS ?= --players 10 --closed-announcements --brief-log

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
.SHELLFLAGS := /C
EXECUTABLE_SUFFIX := .exe
RUN_PREFIX :=
CLEAN_PROGRAM := del /Q
ECHO := echo
else
EXECUTABLE_SUFFIX :=
RUN_PREFIX := ./
CLEAN_PROGRAM := rm -f
ECHO := echo
endif

TARGET := mafia$(EXECUTABLE_SUFFIX)
TEST_TARGET := shared_ptr_tests$(EXECUTABLE_SUFFIX)
PLAYER_TEST_TARGET := player_tests$(EXECUTABLE_SUFFIX)
ROLE_TEST_TARGET := role_tests$(EXECUTABLE_SUFFIX)
HOST_TEST_TARGET := host_voting_tests$(EXECUTABLE_SUFFIX)
NIGHT_TEST_TARGET := host_night_tests$(EXECUTABLE_SUFFIX)
GAME_TEST_TARGET := game_voting_tests$(EXECUTABLE_SUFFIX)
GAME_CYCLE_TEST_TARGET := game_cycle_tests$(EXECUTABLE_SUFFIX)
GAME_CREATION_TEST_TARGET := game_creation_tests$(EXECUTABLE_SUFFIX)
COMMAND_LINE_TEST_TARGET := command_line_tests$(EXECUTABLE_SUFFIX)
CONSOLE_STRATEGY_TEST_TARGET := console_strategy_tests$(EXECUTABLE_SUFFIX)
GAME_LOGGER_TEST_TARGET := game_logger_tests$(EXECUTABLE_SUFFIX)
ROLE_CONCEPTS_TEST_TARGET := role_concepts_tests$(EXECUTABLE_SUFFIX)
SOURCES := src/main.cpp src/command_line.cpp src/console_strategy.cpp src/game.cpp \
	src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp \
	src/role_config.cpp src/roles.cpp
BINARIES := $(TARGET) $(TEST_TARGET) $(PLAYER_TEST_TARGET) $(ROLE_TEST_TARGET) \
	$(HOST_TEST_TARGET) $(NIGHT_TEST_TARGET) $(GAME_TEST_TARGET) \
	$(GAME_CYCLE_TEST_TARGET) $(GAME_CREATION_TEST_TARGET) \
	$(COMMAND_LINE_TEST_TARGET) $(CONSOLE_STRATEGY_TEST_TARGET) \
	$(GAME_LOGGER_TEST_TARGET) $(ROLE_CONCEPTS_TEST_TARGET)
CLEAN_COMMAND := $(CLEAN_PROGRAM) $(BINARIES)

.PHONY: all run test check clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@$(ECHO) Building application...
	@$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): tests/shared_ptr_tests.cpp include/mafia/shared_ptr.hpp
	@$(ECHO) Building SharedPtr tests...
	@$(CXX) $(CXXFLAGS) tests/shared_ptr_tests.cpp -o $(TEST_TARGET)

$(PLAYER_TEST_TARGET): tests/player_tests.cpp src/host.cpp src/player.cpp
	@$(ECHO) Building Player tests...
	@$(CXX) $(CXXFLAGS) tests/player_tests.cpp src/host.cpp src/player.cpp -o $(PLAYER_TEST_TARGET)

$(ROLE_TEST_TARGET): tests/role_tests.cpp src/host.cpp src/player.cpp src/roles.cpp
	@$(ECHO) Building role tests...
	@$(CXX) $(CXXFLAGS) tests/role_tests.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(ROLE_TEST_TARGET)

$(HOST_TEST_TARGET): tests/host_voting_tests.cpp src/host.cpp src/player.cpp src/roles.cpp
	@$(ECHO) Building Host voting tests...
	@$(CXX) $(CXXFLAGS) tests/host_voting_tests.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(HOST_TEST_TARGET)

$(NIGHT_TEST_TARGET): tests/host_night_tests.cpp src/host.cpp src/player.cpp src/roles.cpp
	@$(ECHO) Building Host night tests...
	@$(CXX) $(CXXFLAGS) tests/host_night_tests.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(NIGHT_TEST_TARGET)

$(GAME_TEST_TARGET): tests/game_voting_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp
	@$(ECHO) Building Game voting tests...
	@$(CXX) $(CXXFLAGS) tests/game_voting_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp -o $(GAME_TEST_TARGET)

$(GAME_CYCLE_TEST_TARGET): tests/game_cycle_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp
	@$(ECHO) Building Game cycle tests...
	@$(CXX) $(CXXFLAGS) tests/game_cycle_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp -o $(GAME_CYCLE_TEST_TARGET)

$(GAME_CREATION_TEST_TARGET): tests/game_creation_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp
	@$(ECHO) Building Game creation tests...
	@$(CXX) $(CXXFLAGS) tests/game_creation_tests.cpp src/console_strategy.cpp src/game.cpp src/game_logger.cpp src/host.cpp src/player.cpp src/random_strategy.cpp src/role_config.cpp src/roles.cpp -o $(GAME_CREATION_TEST_TARGET)

$(COMMAND_LINE_TEST_TARGET): tests/command_line_tests.cpp src/command_line.cpp
	@$(ECHO) Building command line tests...
	@$(CXX) $(CXXFLAGS) tests/command_line_tests.cpp src/command_line.cpp -o $(COMMAND_LINE_TEST_TARGET)

$(CONSOLE_STRATEGY_TEST_TARGET): tests/console_strategy_tests.cpp src/console_strategy.cpp
	@$(ECHO) Building console strategy tests...
	@$(CXX) $(CXXFLAGS) tests/console_strategy_tests.cpp src/console_strategy.cpp -o $(CONSOLE_STRATEGY_TEST_TARGET)

$(GAME_LOGGER_TEST_TARGET): tests/game_logger_tests.cpp src/game_logger.cpp
	@$(ECHO) Building game logger tests...
	@$(CXX) $(CXXFLAGS) tests/game_logger_tests.cpp src/game_logger.cpp -o $(GAME_LOGGER_TEST_TARGET)

$(ROLE_CONCEPTS_TEST_TARGET): tests/role_concepts_tests.cpp include/mafia/role_concepts.hpp include/mafia/roles.hpp
	@$(ECHO) Building role concepts tests...
	@$(CXX) $(CXXFLAGS) tests/role_concepts_tests.cpp -o $(ROLE_CONCEPTS_TEST_TARGET)

test: $(TEST_TARGET) $(PLAYER_TEST_TARGET) $(ROLE_TEST_TARGET) $(HOST_TEST_TARGET) $(NIGHT_TEST_TARGET) $(GAME_TEST_TARGET) $(GAME_CYCLE_TEST_TARGET) $(GAME_CREATION_TEST_TARGET) $(COMMAND_LINE_TEST_TARGET) $(CONSOLE_STRATEGY_TEST_TARGET) $(GAME_LOGGER_TEST_TARGET) $(ROLE_CONCEPTS_TEST_TARGET)
	@$(ECHO) [1/12] SharedPtr tests...
	@$(RUN_PREFIX)$(TEST_TARGET)
	@$(ECHO) [1/12] SharedPtr tests: OK
	@$(ECHO) [2/12] Player tests...
	@$(RUN_PREFIX)$(PLAYER_TEST_TARGET)
	@$(ECHO) [2/12] Player tests: OK
	@$(ECHO) [3/12] Role tests...
	@$(RUN_PREFIX)$(ROLE_TEST_TARGET)
	@$(ECHO) [3/12] Role tests: OK
	@$(ECHO) [4/12] Host voting tests...
	@$(RUN_PREFIX)$(HOST_TEST_TARGET)
	@$(ECHO) [4/12] Host voting tests: OK
	@$(ECHO) [5/12] Host night tests...
	@$(RUN_PREFIX)$(NIGHT_TEST_TARGET)
	@$(ECHO) [5/12] Host night tests: OK
	@$(ECHO) [6/12] Game voting tests...
	@$(RUN_PREFIX)$(GAME_TEST_TARGET)
	@$(ECHO) [6/12] Game voting tests: OK
	@$(ECHO) [7/12] Game cycle tests...
	@$(RUN_PREFIX)$(GAME_CYCLE_TEST_TARGET)
	@$(ECHO) [7/12] Game cycle tests: OK
	@$(ECHO) [8/12] Game creation tests...
	@$(RUN_PREFIX)$(GAME_CREATION_TEST_TARGET)
	@$(ECHO) [8/12] Game creation tests: OK
	@$(ECHO) [9/12] Command line tests...
	@$(RUN_PREFIX)$(COMMAND_LINE_TEST_TARGET)
	@$(ECHO) [9/12] Command line tests: OK
	@$(ECHO) [10/12] Console strategy tests...
	@$(RUN_PREFIX)$(CONSOLE_STRATEGY_TEST_TARGET)
	@$(ECHO) [10/12] Console strategy tests: OK
	@$(ECHO) [11/12] Game logger tests...
	@$(RUN_PREFIX)$(GAME_LOGGER_TEST_TARGET)
	@$(ECHO) [11/12] Game logger tests: OK
	@$(ECHO) [12/12] Role concepts tests...
	@$(RUN_PREFIX)$(ROLE_CONCEPTS_TEST_TARGET)
	@$(ECHO) [12/12] Role concepts tests: OK
	@$(ECHO) All tests passed.

run: $(TARGET)
	@$(ECHO) Running application...
	@$(RUN_PREFIX)$(TARGET) $(RUN_ARGS)

check: test run

clean:
	-$(CLEAN_COMMAND)
