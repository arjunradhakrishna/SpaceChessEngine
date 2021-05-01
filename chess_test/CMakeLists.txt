cmake_minimum_required (VERSION 3.8)

add_executable (chess_test "chess_test.cpp" "algo_dumbo_test.cpp" "test_positions.h" "test_positions.cpp")
target_link_libraries (chess_test common chess algo_linear gtest_main)
add_test (NAME chess_test COMMAND chess_test)

configure_file("games/from_position.pgn"
               "${CMAKE_CURRENT_BINARY_DIR}/games/from_position.pgn"
               COPYONLY)

configure_file("games/lichess_db_standard_rated_2013-01.pgn"
               "${CMAKE_CURRENT_BINARY_DIR}/games/lichess_db_standard_rated_2013-01.pgn"
               COPYONLY)

