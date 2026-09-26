cmake_minimum_required(VERSION 3.20)

project(mafia LANGUAGES CXX)

include(CTest)

add_executable(mafia
    src/game.cpp
    src/host.cpp
    src/main.cpp
    src/player.cpp
    src/roles.cpp
)

target_include_directories(mafia PRIVATE include)
target_compile_features(mafia PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(mafia PRIVATE /W4 /permissive-)
else()
    target_compile_options(mafia PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (BUILD_TESTING)
    add_executable(shared_ptr_tests tests/shared_ptr_tests.cpp)
    target_include_directories(shared_ptr_tests PRIVATE include)
    target_compile_features(shared_ptr_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(shared_ptr_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(shared_ptr_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME shared_ptr_tests COMMAND shared_ptr_tests)

    add_executable(
        player_tests
        tests/player_tests.cpp
        src/host.cpp
        src/player.cpp
    )
    target_include_directories(player_tests PRIVATE include)
    target_compile_features(player_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(player_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(player_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME player_tests COMMAND player_tests)

    add_executable(
        role_tests
        tests/role_tests.cpp
        src/host.cpp
        src/player.cpp
        src/roles.cpp
    )
    target_include_directories(role_tests PRIVATE include)
    target_compile_features(role_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(role_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(role_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME role_tests COMMAND role_tests)

    add_executable(
        host_voting_tests
        tests/host_voting_tests.cpp
        src/host.cpp
        src/player.cpp
        src/roles.cpp
    )
    target_include_directories(host_voting_tests PRIVATE include)
    target_compile_features(host_voting_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(host_voting_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(host_voting_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME host_voting_tests COMMAND host_voting_tests)

    add_executable(
        game_voting_tests
        tests/game_voting_tests.cpp
        src/game.cpp
        src/host.cpp
        src/player.cpp
        src/roles.cpp
    )
    target_include_directories(game_voting_tests PRIVATE include)
    target_compile_features(game_voting_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(game_voting_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(game_voting_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME game_voting_tests COMMAND game_voting_tests)
endif()
