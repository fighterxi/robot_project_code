cmake_minimum_required(VERSION 2.8)

SET(UTILITE_VERSION "0.3.0")
ADD_DEFINITIONS(-DUTILITE_VERSION="${UTILITE_VERSION}")

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")

if(UNIX AND NOT ANDROID)
   FIND_PACKAGE(Pthreads REQUIRED)
ENDIF(UNIX AND NOT ANDROID)

ADD_SUBDIRECTORY( src )
