cmake_minimum_required(VERSION 3.20)
project(xcutor_inject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(xcutor-inject injector.cpp)

target_compile_definitions(xcutor-inject PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
    NOMINMAX
)

# Console subsystem with wide entry point.
if(MSVC)
    target_compile_options(xcutor-inject PRIVATE /W4 /WX /EHsc)
    target_link_options(xcutor-inject PRIVATE /SUBSYSTEM:CONSOLE /ENTRY:wmainCRTStartup)
else()
    target_compile_options(xcutor-inject PRIVATE -Wall -Wextra -Werror -municode)
endif()
