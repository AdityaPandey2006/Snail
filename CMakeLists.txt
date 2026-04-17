cmake_minimum_required(VERSION 3.16)

project(Snail LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

file(GLOB SNAIL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(snail ${SNAIL_SOURCES})

target_include_directories(snail PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(snail PRIVATE -Wall -Wextra)
endif()

