cmake_minimum_required(VERSION 3.10) # Specify minimum required CMake version (adjust as needed)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}) # set output to buidl dir

project(TradingApp) # Define your project name

file(GLOB SRCS "*.cpp" src/*.cpp)
add_executable(TradingApp ${SRCS})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TradingApp)

# Include directories
target_include_directories(TradingApp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
