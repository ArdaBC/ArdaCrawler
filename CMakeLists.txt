cmake_minimum_required(VERSION 3.10)
project(HashtagCollage)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(main_exe src/main.cpp)

add_subdirectory(tests)
