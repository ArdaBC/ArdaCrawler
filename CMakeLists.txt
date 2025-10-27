cmake_minimum_required(VERSION 3.10)
project(ArdaCrawler)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)

include_directories(${PROJECT_SOURCE_DIR}/external/curl/include)

add_executable(main_exe src/main.cpp)

target_link_libraries(main_exe
    ${PROJECT_SOURCE_DIR}/external/curl/lib/libcurl.dll.a
    ${PROJECT_SOURCE_DIR}/external/curl/lib/libssl.a
    ${PROJECT_SOURCE_DIR}/external/curl/lib/libcrypto.a
    ${PROJECT_SOURCE_DIR}/external/curl/lib/libz.a
)

# Post-build: copy libcurl DLL to output folder
add_custom_command(TARGET main_exe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/external/curl/bin/libcurl-x64.dll
        $<TARGET_FILE_DIR:main_exe>
)

add_subdirectory(tests)
