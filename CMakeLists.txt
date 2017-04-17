# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project(stitcher)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(OpenCV REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

if(CMAKE_VERSION VERSION_LESS "2.8.11")
  # Add OpenCV headers location to your include paths
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Save libs and executables in the same place
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Output directory for applications")
#set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

include_directories(./include)
aux_source_directory(./src Stitcher_SRC)

# Declare the executable target built from your sources
add_executable(Picture_Stitch ${Stitcher_SRC})

# Link your application with OpenCV libraries
target_link_libraries(Picture_Stitch ${OpenCV_LIBS})
target_link_libraries(Picture_Stitch event)

set(CMAKE_CXX_COMPILER      "g++")

#set(CMAKE_CXX_FLAGS   "-std=c++11")
set(CMAKE_CXX_FLAGS   "-g")
set(CMAKE_CXX_FLAGS   "-Wall")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

