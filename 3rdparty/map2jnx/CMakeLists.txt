
find_package(GDAL REQUIRED)
find_package(PROJ REQUIRED)
find_package(JPEG REQUIRED)

IF(WIN32)
  LINK_DIRECTORIES( ${CMAKE_SOURCE_DIR}/win32 )
  INCLUDE_DIRECTORIES( ../win32 )
ENDIF(WIN32)


#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(SRCS main.cpp argv.cpp)
SET(HDRS argv.h)


include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GDAL_INCLUDE_DIRS}
  ${PROJ_INCLUDE_DIRS}
  ${JPEG_INCLUDE_DIRS}
)

if(WIN32)
    include_directories(
        ${CMAKE_SOURCE_DIR}/Win32/
    )
endif(WIN32)

#list all source files here
ADD_EXECUTABLE( map2jnx ${SRCS} ${HDRS})

#add definitions, compiler switches, etc.
IF(UNIX)
  ADD_DEFINITIONS(-Wall)  
ENDIF(UNIX)

IF(WIN32)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(WIN32)

TARGET_LINK_LIBRARIES(map2jnx ${GDAL_LIBRARIES} ${PROJ_LIBRARIES} ${JPEG_LIBRARIES})

if(APPLE)
  set_target_properties(map2jnx PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MAC_BUNDLE_PATH}/Contents/Resources
    )
endif(APPLE)

install(
    TARGETS map2jnx DESTINATION ${BIN_INSTALL_DIR}
)

