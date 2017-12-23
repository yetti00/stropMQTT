cmake_minimum_required(VERSION 3.8)
project(stropMQTT)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        stropMQTT.ino)

include_directories(. "c:/Program Files (x86)/Arduino/libraries")

add_executable(stropMQTT ${SOURCE_FILES})