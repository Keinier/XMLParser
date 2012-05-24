#****************************************************************************
#
# CMakeLists.txt file for XMLParser.
# Keinier Caboverde
# http://www.kei-software.com
#
# This is a CMake file cross plataform.
#****************************************************************************
PROJECT(XMLParser)
cmake_minimum_required(VERSION 2.6)

# List of flies headers
SET(HEADER_FILES
        include/xmlparser.h
        include/xmlstring.h)

# List of sources
SET(SOURCE_FILES
        src/xmlerror.cpp
        src/xmlparser.cpp
        src/_xmlparser.cpp
        src/xmlstring.cpp)

SET(USE_STL TRUE)
# Inluce directories
INCLUDE_DIRECTORIES(include/)

# Is Debug or Release?

if(CMAKE_BUILD_TYPE EQUAL DEBUG)
        ADD_DEFINITIONS(-DDEBUG)
        SET(CMAKE_C_FLAGS_DEBUG -Wall -Wno-format -g)
else(CMAKE_BUILD_TYPE EQUAL DEBUG)
        ADD_DEFINITIONS(-DRELEASE)
        SET(CMAKE_C_FLAGS_DEBUG -Wall -Wno-unknown-pragmas -Wno-format -O3)
endif(CMAKE_BUILD_TYPE EQUAL DEBUG)

# Use STL Libraries for compilation?
if(USE_STL)
        ADD_DEFINITIONS(-DUSE_STL)
endif(USE_STL)

# Out path information
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Build static library
ADD_LIBRARY(XMLParser STATIC ${SOURCE_FILES})



