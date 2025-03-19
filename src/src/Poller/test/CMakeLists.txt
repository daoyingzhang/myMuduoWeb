add_executable(test_time test_time.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Poller/test)

target_link_libraries(test_time myweb)
