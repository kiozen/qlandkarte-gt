project(tzone)

# Required cmake version
cmake_minimum_required(VERSION 2.6.0)


set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


if(QK_QT5_PORT)
  find_package(Qt5Widgets)
else(QK_QT5_PORT)
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif(QK_QT5_PORT)



set(SRCS
    zonefiles.cpp
    src/tzfile.cpp
    src/tzdata.cpp
)

if(WIN32)
    SET(SRCS ${SRCS}  src/tzsys_win.cpp)
else(WIN32)
    SET(SRCS ${SRCS}  src/tzsys_unix.cpp)
endif(WIN32)


set(HDRS
    include/tzfile.h
    include/tzdata.h
    src/tzsys.h
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
  ${CMAKE_SOURCE_DIR}/3rdparty/tzone/include
)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)

add_library(tzone
  ${SRCS}
  ${HDRS}
  ${UI_HDRS}
  ${RC_SRCS}
  ${MOC_SRCS}
  ${QM_FILES}
)
if(QK_QT5_PORT)
  qt5_use_modules(tzone Widgets)
endif(QK_QT5_PORT)

if(WIN32)
target_link_libraries( tzone
      ${QT_LIBRARIES}
)
else(WIN32)
target_link_libraries( tzone
      ${QT_LIBRARIES}
)
endif(WIN32)
