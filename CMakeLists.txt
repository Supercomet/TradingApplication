cmake_minimum_required(VERSION 3.10) # Specify minimum required CMake version (adjust as needed)

project(TradingApp) # Define your project name

#
# CMake setup
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}) # set output to buidl dir
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_VERBOSE_MAKEFILE 0) # 1 should be used for debugging
set (CMAKE_SUPPRESS_REGENERATION TRUE) # Suppresses ZERO_CHECK
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
  if(NOT WIN32)
    set(GLAD_LIBRARIES dl)
  endif()
endif()


set(SDL_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/TradingApp/external/sdl2)
set(GLAD_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/TradingApp/external/glad)
set(IMGUI_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/TradingApp/external/imgui)
set(IMPLOT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/TradingApp/external/implot)
#
# Set include paths
#
include_directories (TradingApp/src/
                     ${GLAD_PATH}/include/
                     ${SDL_PATH}/include/
                     ${SDL_PATH}/include/sdl2
                     TradingApp/external/glm/
					 ${IMGUI_PATH}/
					 ${IMPLOT_PATH}/
					 )
#                     TradingApp/external/stb/
					 


#
# Add files
#
file (GLOB VENDORS_SOURCES ${GLAD_PATH}/src/*.c
								   ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
								   ${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp
								   ${IMGUI_PATH}/*.cpp
								   ${IMPLOT_PATH}/*.cpp
									)
file (GLOB_RECURSE PROJECT_HEADERS TradingApp/src/*.hpp
                                   TradingApp/src/*.h)
file (GLOB_RECURSE PROJECT_SOURCES TradingApp/src/*.cpp
                                   TradingApp/src/*.cxx
                                   TradingApp/src/*.cc
                                   TradingApp/src/*.c)
file (GLOB_RECURSE PROJECT_SHADERS TradingApp/shaders/*.comp
                                   TradingApp/shaders/*.frag
                                   TradingApp/shaders/*.geom
                                   TradingApp/shaders/*.vert)
file (GLOB         PROJECT_CONFIGS CMakeLists.txt
                                   README.rst
                                  .gitignore
                                  .gitmodules)
								  
add_definitions (-DGLFW_INCLUDE_NONE
                 -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable (${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
                                ${PROJECT_SHADERS} 
								#${PROJECT_CONFIGS}
                                ${VENDORS_SOURCES})
target_link_libraries (${PROJECT_NAME}
                       ${SDL_PATH}/lib/x64/sdl2.lib
                       ${GLAD_LIBRARIES})
set_target_properties (${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})


#  set(GLM_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/external/glm)
#  include_directories(${GLM_PATH})  # include glm
#  

#  include_directories(${GLAD_PATH}/include)  # include glad
#  file(GLOB GLAD_SRCS "*.c" ${GLAD_PATH}/src/*.c)
#  
 
#  include_directories(${SDL_PATH}/include)  # include sdl2


#   add_executable(TradingApp ${SRCS} ${GLAD_SRCS})
#   
#   # Link to the actual SDL2 library.
#   target_link_libraries(TradingApp PRIVATE ${SDL_PATH}/lib/x64/sdl2.lib)
#   
#   # Include directories
#   target_include_directories(TradingApp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
#   
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TradingApp) # set startup proj for convenience
