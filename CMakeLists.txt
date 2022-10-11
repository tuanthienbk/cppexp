cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GCC_CONCEPTS_DIAGNOSTICS_DEPTH "-fconcepts-diagnostics-depth=2")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CONCEPTS_DIAGNOSTICS_DEPTH}")
set(CMAKE_C_COMPILER /usr/local/bin/gcc-12)
set(CMAKE_CXX_COMPILER /usr/local/bin/g++-12)

project(test VERSION 0.1.0)

set(SOURCES
main.cpp
queue_test.cpp
threadpool_test.cpp
task_test.cpp
generator_test.cpp
stream_test.cpp
)

set(HEADERS
lock.h
concurrent_queue.h
threadpool.h
task.h
generator.h
stream.h
)
add_subdirectory(googletest)
include_directories(SYSTEM googletest/include)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE gtest)
