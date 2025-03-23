add_executable(memory_test memoryTest.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Memory/test)

target_link_libraries(memory_test myweb)
