CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -pthread -Iinclude
TARGET := mafia
TEST_TARGET := shared_ptr_tests
PLAYER_TEST_TARGET := player_tests
ROLE_TEST_TARGET := role_tests
HOST_TEST_TARGET := host_voting_tests
GAME_TEST_TARGET := game_voting_tests
SOURCES := src/main.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): tests/shared_ptr_tests.cpp include/mafia/shared_ptr.hpp
	$(CXX) $(CXXFLAGS) tests/shared_ptr_tests.cpp -o $(TEST_TARGET)

$(PLAYER_TEST_TARGET): tests/player_tests.cpp src/host.cpp src/player.cpp
	$(CXX) $(CXXFLAGS) tests/player_tests.cpp src/host.cpp src/player.cpp -o $(PLAYER_TEST_TARGET)

$(ROLE_TEST_TARGET): tests/role_tests.cpp src/host.cpp src/player.cpp src/roles.cpp
	$(CXX) $(CXXFLAGS) tests/role_tests.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(ROLE_TEST_TARGET)

$(HOST_TEST_TARGET): tests/host_voting_tests.cpp src/host.cpp src/player.cpp src/roles.cpp
	$(CXX) $(CXXFLAGS) tests/host_voting_tests.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(HOST_TEST_TARGET)

$(GAME_TEST_TARGET): tests/game_voting_tests.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp
	$(CXX) $(CXXFLAGS) tests/game_voting_tests.cpp src/game.cpp src/host.cpp src/player.cpp src/roles.cpp -o $(GAME_TEST_TARGET)

test: $(TEST_TARGET) $(PLAYER_TEST_TARGET) $(ROLE_TEST_TARGET) $(HOST_TEST_TARGET) $(GAME_TEST_TARGET)
	./$(TEST_TARGET)
	./$(PLAYER_TEST_TARGET)
	./$(ROLE_TEST_TARGET)
	./$(HOST_TEST_TARGET)
	./$(GAME_TEST_TARGET)

clean:
	$(RM) $(TARGET) $(TARGET).exe $(TEST_TARGET) $(TEST_TARGET).exe \
		$(PLAYER_TEST_TARGET) $(PLAYER_TEST_TARGET).exe \
		$(ROLE_TEST_TARGET) $(ROLE_TEST_TARGET).exe \
		$(HOST_TEST_TARGET) $(HOST_TEST_TARGET).exe \
		$(GAME_TEST_TARGET) $(GAME_TEST_TARGET).exe
