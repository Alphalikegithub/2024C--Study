cmake_minimum_required(VERSION 3.5)
project(SORTLIB)
aux_source_directory(./ SRC)
include_directories(${HEADPATH})
set(LIBRARY_OUTPUT_PATH ${LIBPATH})
add_library(${SORTLIB} SHARED ${SRC})
