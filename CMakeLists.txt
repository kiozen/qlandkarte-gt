project(QLandkarteGT)

option(QK_QT5_PORT "Qt5 port" OFF)
if(QK_QT5_PORT)
  set(QT_QMAKE_EXECUTABLE "/usr/lib64/qt5/bin/qmake")
  add_definitions(-DQK_QT5_PORT)
  cmake_minimum_required(VERSION 2.8.9)
  find_package(Qt5Widgets)
  if(NOT(${Qt5Core_VERSION_MINOR} LESS 1))
    set(QK_QT5_SERIAL_PORT ON)
    set(QEXTSERIALPORT_INCLUDE_DIRS "")
  endif(NOT(${Qt5Core_VERSION_MINOR} LESS 1))
  if(NOT(${Qt5Core_VERSION_MINOR} LESS 2))
    set(QK_QT5_TZONE ON)
  endif(NOT(${Qt5Core_VERSION_MINOR} LESS 2))
else(QK_QT5_PORT)
  cmake_minimum_required(VERSION 2.6.0)
endif(QK_QT5_PORT)


set(APPLICATION_NAME ${PROJECT_NAME})

set(APPLICATION_VERSION_MAJOR "1")
set(APPLICATION_VERSION_MINOR "8")
set(APPLICATION_VERSION_PATCH "1")


add_definitions(-DVER_MAJOR=${APPLICATION_VERSION_MAJOR} -DVER_MINOR=${APPLICATION_VERSION_MINOR} -DVER_STEP=${APPLICATION_VERSION_PATCH})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
if (APPLE)
  set(PROGNAME "QLandkarte GT")
  set(CONFIGDIR "Library/Application Support/${PROGNAME}/")
  set(MAC_BUNDLE_PATH ${EXECUTABLE_OUTPUT_PATH}/${PROGNAME}.app)
  set(MAC_ICON_FILE qlandkartegt.icns)
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH})
  set(MACOSX_BUNDLE_ICON_FILE ${MAC_ICON_FILE})
  set(MACOSX_BUNDLE_BUNDLE_NAME ${PROGNAME})
  set(MACOSX_BUNDLE_BUNDLE_VERSION ${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER org.qlandkarte.QLandkarteGT)
else(APPLE)
  set(PROGNAME qlandkartegt)
  set(CONFIGDIR ".config/QLandkarteGT/")
endif(APPLE)
if(MSVC)
  # Add link-time code generation to improve performance
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
  set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
endif(MSVC)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

# add definitions
include(DefineCMakeDefaults)
include(DefineCompilerFlags)
include(DefineInstallationPaths)
include(CPackConfig.cmake)

# config.h generation
if (NOT WIN32)
SET (NATIVE_FILE_DIALOG ON CACHE BOOL "Use OS native file dialog")
else (NOT WIN32)
SET (NATIVE_FILE_DIALOG OFF CACHE BOOL "Use OS native file dialog")
endif (NOT WIN32)

if(NATIVE_FILE_DIALOG)
    set(FILEDIALOGFLAGS "0")
else(NATIVE_FILE_DIALOG)
    set(FILEDIALOGFLAGS "QFileDialog::DontUseNativeDialog")
endif(NATIVE_FILE_DIALOG)

include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if (NOT QK_QT5_SERIAL_PORT)
  if (NOT WIN32)
    find_package(QextSerialPort)
  else (NOT WIN32)
    find_package(QextSerialPort QUIET)
  endif (NOT WIN32)

  if(NOT QEXTSERIALPORT_FOUND)
    add_subdirectory(3rdparty/SerialPort/qextserialport)
    set(QEXTSERIALPORT_LIBRARIES qextserialport)
    set(QEXTSERIALPORT_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/SerialPort/qextserialport)
  endif()
  add_subdirectory(3rdparty/SerialPort)
endif (NOT QK_QT5_SERIAL_PORT)

add_subdirectory(3rdparty/QZip)
add_subdirectory(3rdparty/QTextHtmlExporter)
add_subdirectory(3rdparty/getopt)
add_subdirectory(3rdparty/map2gcm)
add_subdirectory(3rdparty/map2rmap)
add_subdirectory(3rdparty/map2rmp)
add_subdirectory(3rdparty/map2jnx)
add_subdirectory(3rdparty/cache2gtiff)

if (NOT QK_QT5_TZONE)
    add_subdirectory(3rdparty/tzone)
endif (NOT QK_QT5_TZONE)

find_package(QtSoap)
if (NOT QTSOAP_FOUND)
    add_subdirectory(3rdparty/qtsoap)
endif()

add_subdirectory(src)

if (UNIX AND NOT WIN32 AND NOT APPLE)
  install(
    FILES
      qlandkartegt.desktop
    DESTINATION
      ${XDG_APPS_DIR}
  )

  install(
    FILES
      src/icons/qlandkartegt.png
    DESTINATION
      ${DATA_INSTALL_PREFIX}/pixmaps
  )

  install(
    FILES
      qlandkartegt.1
    DESTINATION
      ${MAN_INSTALL_DIR}/man1
  )
endif (UNIX AND NOT WIN32 AND NOT APPLE)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


