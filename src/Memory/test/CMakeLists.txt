add_executable(memory_test memoryTest.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Memory/test)

target_link_libraries(memory_test myweb)


# add_executable(test_e test.cpp)

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Mysql/test)

# target_link_libraries(test_e myweb)
