cmake_minimum_required(VERSION 3.20)

project(mafia LANGUAGES CXX)

include(CTest)

add_executable(mafia
    src/ai_strategy.cpp
    src/command_line.cpp
    src/console_strategy.cpp
    src/game.cpp
    src/game_logger.cpp
    src/host.cpp
    src/main.cpp
    src/openai_llm_client.cpp
    src/player.cpp
    src/random_strategy.cpp
    src/role_config.cpp
    src/roles.cpp
)

target_include_directories(mafia PRIVATE include)
target_compile_features(mafia PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(mafia PRIVATE /W4 /permissive-)
else()
    target_compile_options(mafia PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_link_libraries(mafia PRIVATE winhttp)
endif()

if (BUILD_TESTING)
    add_executable(
        ai_strategy_tests
        tests/ai_strategy_tests.cpp
        src/ai_strategy.cpp
    )
    target_include_directories(ai_strategy_tests PRIVATE include)
    target_compile_features(ai_strategy_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(ai_strategy_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ai_strategy_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ai_strategy_tests COMMAND ai_strategy_tests)

    add_executable(
        command_line_tests
        tests/command_line_tests.cpp
        src/command_line.cpp
    )
    target_include_directories(command_line_tests PRIVATE include)
    target_compile_features(command_line_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(command_line_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(command_line_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME command_line_tests COMMAND command_line_tests)

    add_executable(
        console_strategy_tests
        tests/console_strategy_tests.cpp
        src/console_strategy.cpp
    )
    target_include_directories(console_strategy_tests PRIVATE include)
    target_compile_features(console_strategy_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(console_strategy_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(console_strategy_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME console_strategy_tests COMMAND console_strategy_tests)

    add_executable(
        game_logger_tests
        tests/game_logger_tests.cpp
        src/game_logger.cpp
    )
    target_include_directories(game_logger_tests PRIVATE include)
    target_compile_features(game_logger_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(game_logger_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(game_logger_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME game_logger_tests COMMAND game_logger_tests)

    add_executable(role_concepts_tests tests/role_concepts_tests.cpp)
    target_include_directories(role_concepts_tests PRIVATE include)
    target_compile_features(role_concepts_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(role_concepts_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(role_concepts_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME role_concepts_tests COMMAND role_concepts_tests)

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
        host_night_tests
        tests/host_night_tests.cpp
        src/host.cpp
        src/player.cpp
        src/roles.cpp
    )
    target_include_directories(host_night_tests PRIVATE include)
    target_compile_features(host_night_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(host_night_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(host_night_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME host_night_tests COMMAND host_night_tests)

    add_executable(
        game_voting_tests
        tests/game_voting_tests.cpp
        src/ai_strategy.cpp
        src/console_strategy.cpp
        src/game.cpp
        src/game_logger.cpp
        src/host.cpp
        src/openai_llm_client.cpp
        src/player.cpp
        src/random_strategy.cpp
        src/role_config.cpp
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

    if (WIN32)
        target_link_libraries(game_voting_tests PRIVATE winhttp)
    endif()

    add_executable(
        game_cycle_tests
        tests/game_cycle_tests.cpp
        src/ai_strategy.cpp
        src/console_strategy.cpp
        src/game.cpp
        src/game_logger.cpp
        src/host.cpp
        src/openai_llm_client.cpp
        src/player.cpp
        src/random_strategy.cpp
        src/role_config.cpp
        src/roles.cpp
    )
    target_include_directories(game_cycle_tests PRIVATE include)
    target_compile_features(game_cycle_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(game_cycle_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(game_cycle_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME game_cycle_tests COMMAND game_cycle_tests)

    if (WIN32)
        target_link_libraries(game_cycle_tests PRIVATE winhttp)
    endif()

    add_executable(
        game_creation_tests
        tests/game_creation_tests.cpp
        src/ai_strategy.cpp
        src/console_strategy.cpp
        src/game.cpp
        src/game_logger.cpp
        src/host.cpp
        src/openai_llm_client.cpp
        src/player.cpp
        src/random_strategy.cpp
        src/role_config.cpp
        src/roles.cpp
    )
    target_include_directories(game_creation_tests PRIVATE include)
    target_compile_features(game_creation_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(game_creation_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(game_creation_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME game_creation_tests COMMAND game_creation_tests)
    set_tests_properties(
        game_creation_tests
        PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    if (WIN32)
        target_link_libraries(game_creation_tests PRIVATE winhttp)
    endif()
endif()
