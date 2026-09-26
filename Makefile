CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -pthread -Iinclude

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
GAME_TEST_TARGET := game_voting_tests$(EXECUTABLE_SUFFIX)
SOURCES := src/main.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp
BINARIES := $(TARGET) $(TEST_TARGET) $(PLAYER_TEST_TARGET) $(ROLE_TEST_TARGET) \
	$(HOST_TEST_TARGET) $(GAME_TEST_TARGET)
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

$(GAME_TEST_TARGET): tests/game_voting_tests.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp
	@$(ECHO) Building Game voting tests...
	@$(CXX) $(CXXFLAGS) tests/game_voting_tests.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(GAME_TEST_TARGET)

test: $(TEST_TARGET) $(PLAYER_TEST_TARGET) $(ROLE_TEST_TARGET) $(HOST_TEST_TARGET) $(GAME_TEST_TARGET)
	@$(ECHO) [1/5] SharedPtr tests...
	@$(RUN_PREFIX)$(TEST_TARGET)
	@$(ECHO) [1/5] SharedPtr tests: OK
	@$(ECHO) [2/5] Player tests...
	@$(RUN_PREFIX)$(PLAYER_TEST_TARGET)
	@$(ECHO) [2/5] Player tests: OK
	@$(ECHO) [3/5] Role tests...
	@$(RUN_PREFIX)$(ROLE_TEST_TARGET)
	@$(ECHO) [3/5] Role tests: OK
	@$(ECHO) [4/5] Host voting tests...
	@$(RUN_PREFIX)$(HOST_TEST_TARGET)
	@$(ECHO) [4/5] Host voting tests: OK
	@$(ECHO) [5/5] Game voting tests...
	@$(RUN_PREFIX)$(GAME_TEST_TARGET)
	@$(ECHO) [5/5] Game voting tests: OK
	@$(ECHO) All tests passed.

run: $(TARGET)
	@$(ECHO) Running application...
	@$(RUN_PREFIX)$(TARGET)

check: test run

clean:
	-$(CLEAN_COMMAND)
