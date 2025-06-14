cmake_minimum_required(VERSION 3.10)
project(MetricLoggerDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

add_executable(demo example/demo.cpp)

enable_testing()
add_test(NAME PythonTests
         COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}
         python3 ${CMAKE_SOURCE_DIR}/tests/test.py)
