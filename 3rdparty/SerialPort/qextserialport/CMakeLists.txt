project(qextserialport)

# Required cmake version
cmake_minimum_required(VERSION 2.8.6)


set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

find_package(Qt4 REQUIRED)

include(${QT_USE_FILE})

set(SRCS
    qextserialport.cpp
)

set(HDRS
    qextserialport.h
)

if(WIN32)
    set(SRCS ${SRCS} qextserialport_win.cpp)
else (WIN32)
    set(SRCS ${SRCS} qextserialport_unix.cpp)
endif(WIN32)


include_directories(
)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)

add_library(qextserialport
  ${SRCS}
  ${HDRS}
)

set_target_properties(qextserialport PROPERTIES AUTOMOC TRUE)

target_link_libraries( qextserialport
      ${QT_LIBRARIES}
)
