add_executable(test_e test.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Mysql/test)

target_link_libraries(test_e myweb)
