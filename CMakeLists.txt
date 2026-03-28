cmake_minimum_required(VERSION 3.24)
project(calliope-native)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(vendor/JUCE)
add_subdirectory(engine)
add_subdirectory(engine/tests)
add_subdirectory(native)
