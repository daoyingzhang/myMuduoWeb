# 加载子目录
set(HTTP_SRCS
  main.cpp
)

add_executable(http_test ${HTTP_SRCS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/Http/test)

target_link_libraries(http_test myweb)
