project(QZip)

# Required cmake version
cmake_minimum_required(VERSION 2.6.0)


set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


if(QK_QT5_PORT)
  find_package(Qt5Widgets)
else(QK_QT5_PORT)
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif(QK_QT5_PORT)

if(WIN32)
    #add path to zlib.h in QT 3rdparty directory
    EXEC_PROGRAM( ${QT_QMAKE_EXECUTABLE}
      ARGS "-query QT_INSTALL_PREFIX"
      OUTPUT_VARIABLE QT_INSTALL_PREFIX )
    # make sure we have / and not \ as qmake gives on windows
    FILE(TO_CMAKE_PATH "${QT_INSTALL_PREFIX}" QT_INSTALL_PREFIX)
    set(QT_ZLIB_INCLUDE_DIR ${QT_INSTALL_PREFIX}/src/3rdparty/zlib)
endif(WIN32)



set(SRCS
    qzip.cpp
)

set(HDRS
    qzipreader.h
    qzipwriter.h
)

set(UIS

)

set(RCS


)


if(QK_QT5_PORT)
  qt5_wrap_cpp(MOC_SRCS ${HDRS})
  qt5_wrap_ui(UI_HDRS ${UIS})
  qt5_add_resources(RC_SRCS ${RCS})
else(QK_QT5_PORT)
  qt4_wrap_cpp(MOC_SRCS ${HDRS})
  qt4_wrap_ui(UI_HDRS ${UIS})
  qt4_add_resources(RC_SRCS ${RCS})
endif(QK_QT5_PORT)



include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_QTXML_INCLUDE_DIR}
  ${QT_ZLIB_INCLUDE_DIR}
)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)

add_library(qzip
  ${SRCS}
  ${HDRS}
  ${UI_HDRS}
  ${RC_SRCS}
  ${MOC_SRCS}
  ${QM_FILES}
)

if(QK_QT5_PORT)
  qt5_use_modules(qzip Widgets)
endif(QK_QT5_PORT)

if(WIN32)
target_link_libraries( qzip
      ${QT_LIBRARIES}
)
else(WIN32)
target_link_libraries( qzip
      ${QT_LIBRARIES}
      z
)
endif(WIN32)
