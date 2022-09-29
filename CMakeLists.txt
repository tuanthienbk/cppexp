cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(test VERSION 0.1.0)

set(SOURCES
main.cpp
queue_test.cpp
threadpool_test.cpp
)

set(HEADERS
lock.h
concurrent_queue.h
threadpool.h
)
add_subdirectory(googletest)
include_directories(SYSTEM googletest/include)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE gtest)
