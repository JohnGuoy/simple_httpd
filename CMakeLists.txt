cmake_minimum_required(VERSION 2.8.4)
project(simple_httpd C)

set(CMAKE_C_STANDARD 99)

add_executable(simple_httpd main.c)

TARGET_LINK_LIBRARIES(simple_httpd pthread)

ADD_DEFINITIONS(-D_REENTRANT)
