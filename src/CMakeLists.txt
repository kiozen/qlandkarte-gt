project(qlandkarteqt-binary)

option(WITH_EXIF "BUILD EXIF" ON)
option(WITH_DMTX "BUILD DMTX" ON)
option(WITH_GPSD "BUILD_GPSD" ON)


if(QK_QT5_PORT)
  find_package(Qt5Widgets)
  find_package(Qt5Quick)
  find_package(Qt5Network)
  find_package(Qt5Gui)
  find_package(Qt5OpenGL)
  find_package(Qt5Xml)
  find_package(Qt5Sql)
  if(NOT APPLE)
    find_package(Qt5DBus)
  endif(NOT APPLE)
  find_package(Qt5PrintSupport)
  find_package(Qt5Script)
  find_package(Qt5Multimedia)
  find_package(Qt5WebKitWidgets)
  if(QK_QT5_SERIAL_PORT)
    find_package(Qt5SerialPort)
    add_definitions(-DQK_QT5_SERIAL_PORT)
  endif(QK_QT5_SERIAL_PORT)
  if(QK_QT5_TZONE)
    add_definitions(-DQK_QT5_TZONE)
  endif(QK_QT5_TZONE)
else(QK_QT5_PORT)
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif(QK_QT5_PORT)

find_package(OpenGL REQUIRED)
find_package(GDAL REQUIRED)
find_package(PROJ REQUIRED)
if(WITH_EXIF)
    find_package(EXIF)
endif(WITH_EXIF)
if(WITH_DMTX)
    find_package(DMTX)
endif(WITH_DMTX)
if(WITH_GPSD)
    find_package(GPSD)
endif(WITH_GPSD)

if (NOT WIN32)
  find_program(GDALWARP gdalwarp DOC "full path of the gdalwarp application")
  find_program(GDALTRANSLATE gdal_translate DOC "full path of the gdal_translate application")
  find_program(GDALADDO gdaladdo DOC "full path of the gdaladdo application")
  find_program(GPSBABEL gpsbabel DOC "full path of the gpsbabel application")
else (NOT WIN32)
  set(GDALWARP "gdalwarp.exe")
  set(GDALTRANSLATE "gdal_translate.exe")
  set(GDALADDO "gdaladdo.exe")
  set(GPSBABEL "gpsbabel.exe")
endif (NOT WIN32)

if(GDALWARP)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALWARP=\\\"${GDALWARP}\\\"")
else(GDALWARP)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALWARP=\\\"gdalwarp\\\"")
endif(GDALWARP)

if(GDALTRANSLATE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALTRANSLATE=\\\"${GDALTRANSLATE}\\\"")
else(GDALTRANSLATE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALTRANSLATE=\\\"gdal_translate\\\"")
endif(GDALTRANSLATE)

if(GDALADDO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALADDO=\\\"${GDALADDO}\\\"")
else(GDALADDO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGDALADDO=\\\"gdaladdo\\\"")
endif(GDALADDO)


if(GPSBABEL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGPSBABEL=\\\"${GPSBABEL}\\\"")
else(GPSBABEL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGPSBABEL=\\\"gpsbabel\\\"")
endif(GPSBABEL)

if(WITH_PROFILING)
    set(PROFILING_LIBRARIES "conan")
else(WITH_PROFILING)
    set(PROFILING_LIBRARIES "")
endif(WITH_PROFILING)


if (NOT WIN32)
  find_program(MAP2JNX map2jnx DOC "full path of the map2jnx application")
  if(MAP2JNX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAP2JNX=\\\"${MAP2JNX}\\\"")
  else(MAP2JNX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAP2JNX=\\\"map2jnx\\\"")
  endif(MAP2JNX)
endif (NOT WIN32)



set(qlandkarte_SRCS
    main.cpp
    mitab.cpp
    tcxreader.cpp
    CActions.cpp
    CMenus.cpp
    GeoMath.cpp
    WptIcons.cpp
    CResources.cpp
    CCopyright.cpp
    CApplication.cpp
    CMainWindow.cpp
    CCanvas.cpp
    CCanvasUndoCommandZoom.cpp
    CStatusDEM.cpp
    IMouse.cpp
    CMouseMoveMap.cpp
    CMouseZoomMap.cpp
    CMouseSelMap.cpp
    CMouseAddWpt.cpp
    CMouseMoveWpt.cpp
    CMouseEditWpt.cpp
    CMouseRefPoint.cpp
    CMouseCutTrack.cpp
    CMouseSelTrack.cpp
    CMouseAddText.cpp
    CMouseAddTextBox.cpp
    CMouseAddDistance.cpp
    CMouseAddArea.cpp
    CMouseOverlay.cpp
    CMouseColorPicker.cpp
    CMouseSelWpt.cpp
    IMap.cpp
    CMapNoMap.cpp
    CMapDEM.cpp
    CMapFile.cpp
    CMapLevel.cpp
    CMapQMAP.cpp
    CMapQMAPExport.cpp
    CMapRaster.cpp
    CMapGeoTiff.cpp
    CMapWms.cpp
    CMapTms.cpp
    CMapJnx.cpp
    CMapRmap.cpp
    CMap3D.cpp
    IMapSelection.cpp
    CMapSelectionRaster.cpp
    CMapSelectionGarmin.cpp
    CMapSearchWidget.cpp
    CMapSearchCanvas.cpp
    CMapSearchThread.cpp
    CMegaMenu.cpp
    IDB.cpp
    CWpt.cpp
    CWptDB.cpp
    CWptToolWidget.cpp
    CSearch.cpp
    CSearchDB.cpp
    CSearchToolWidget.cpp
    CDlgConfig.cpp
    CDlgEditWpt.cpp
    CDlgWptIcon.cpp
    CDlgEditMapLevel.cpp
    CDlgCombineTracks.cpp
    CDlgCombineDistOvl.cpp
    CDlgProjWizzard.cpp
    CDlgEditText.cpp
    CDlgDelWpt.cpp
    CDlgConvertToTrack.cpp
    CDlgEditRoute.cpp
    CDlgLoadOnlineMap.cpp
    CDlgMapTDBConfig.cpp
    CDlgMapQMAPConfig.cpp
    CDlgMapJNXConfig.cpp
    CDlgMapTmsConfig.cpp
    CDlgMapWmsConfig.cpp
    CDlgMapRMPConfig.cpp
    CDlgNoMapConfig.cpp
    CDlgScreenshot.cpp
    CDlgTrackFilter.cpp
    CDlgSetupGarminIcons.cpp
    CDlgConfig3D.cpp
    CDlgExport.cpp
    CDlgWpt2Rte.cpp
    CDlgSetupGrid.cpp
    CDlgCropMap.cpp
    CDlgParentWpt.cpp
    CDlgDeviceExportPath.cpp
    CDlgProxy.cpp
    CDlgMultiColorConfig.cpp
    CDlg3DHelp.cpp
    CQlb.cpp
    CGpx.cpp
    IDevice.cpp
    CDeviceQLandkarteM.cpp
    CDeviceMagellan.cpp
    CDeviceGarmin.cpp
    CDeviceGarminBulk.cpp
    CDeviceNMEA.cpp
    CDeviceTwoNav.cpp
    CCreateMapQMAP.cpp
    CCreateMapGeoTiff.cpp
    CCreateMapGridTool.cpp
    CCreateMapFineTune.cpp
    CMapUndoCommandMove.cpp
    CMapDB.cpp
    CMapToolWidget.cpp
    CMapEditWidget.cpp
    CUndoStackModel.cpp
    CUndoStackView.cpp
    CTrackUndoCommandDeletePts.cpp
    CTrackUndoCommandPurgePts.cpp
    CTrackUndoCommandSelect.cpp
    CTrack.cpp
    CTrackDB.cpp
    CTrackToolWidget.cpp
    CTrackEditWidget.cpp
    ITrackStat.cpp
    CTrackStatProfileWidget.cpp
    CTrackStatSpeedWidget.cpp
    CTrackStatTraineeWidget.cpp
    CTrackStatDistanceWidget.cpp
    CTrackFilterWidget.cpp
    CRoute.cpp
    CRouteDB.cpp
    CRouteToolWidget.cpp
    CTabWidget.cpp
    CDiary.cpp
    CDiaryDB.cpp
    CDiaryEdit.cpp
    CTextEditWidget.cpp
    CPlot.cpp
    CPlotAxis.cpp
    CPlotAxisTime.cpp
    CPlotData.cpp
    CLiveLog.cpp
    CLiveLogDB.cpp
    CLiveLogToolWidget.cpp
    IOverlay.cpp
    COverlayDB.cpp
    COverlayToolWidget.cpp
    COverlayText.cpp
    COverlayTextBox.cpp
    COverlayDistance.cpp
    COverlayDistanceEditWidget.cpp
    COverlayArea.cpp
    COverlayAreaEditWidget.cpp
    IUnit.cpp
    CUnitMetric.cpp
    CUnitImperial.cpp
    CUnitNautic.cpp
    CImage.cpp
    IGarminStrTbl.cpp
    CGarminStrTbl6.cpp
    CGarminStrTbl8.cpp
    CGarminStrTblUtf8.cpp
    CGarminPolygon.cpp
    CGarminPoint.cpp
    CGarminTile.cpp
    CGarminExport.cpp
    IGarminTyp.cpp
    CGarminTyp.cpp
    CGarminTypNT.cpp
    CMapTDB.cpp
    CHelpButton.cpp
    CSpline.cpp
    IItem.cpp
    CImageSelect.cpp
    CGridDB.cpp
    IDiskCache.cpp
    CDiskCache.cpp
    CDiskCacheZip.cpp
    CTextBrowser.cpp
    CImageViewer.cpp
    CMapRmp.cpp
    CMapDEMSlopeSetup.cpp
    IExchange.cpp
    CExchangeGarmin.cpp
    CExchangeTwoNav.cpp
)

if (WIN32)
    SET(qlandkarte_SRCS ${qlandkarte_SRCS} qlandkarte.rc)
endif (WIN32)

set(qlandkarte_HDRS
    Platform.h
    version.h
    mitab.h
    tcxreader.h
    CActions.h
    CMenus.h
    GeoMath.h
    WptIcons.h
    CResources.h
    CCopyright.h
    CApplication.h
    CMainWindow.h
    CCanvas.h
    CStatusDEM.h
    IMouse.h
    CMouseMoveMap.h
    CMouseZoomMap.h
    CMouseSelMap.h
    CMouseAddWpt.h
    CMouseMoveWpt.h
    CMouseEditWpt.h
    CMouseRefPoint.h
    CMouseCutTrack.h
    CMouseSelTrack.h
    CMouseAddText.h
    CMouseAddTextBox.h
    CMouseAddDistance.h
    CMouseAddArea.h
    CMouseOverlay.h
    CMouseColorPicker.h
    CMouseSelWpt.h
    IMap.h
    CMapNoMap.h
    CMapDEM.h
    CMapFile.h
    CMapLevel.h
    CMapQMAP.h
    CMapQMAPExport.h
    CMapRaster.h
    CMapGeoTiff.h
    CMapWms.h
    CMapTms.h
    CMapJnx.h
    CMapRmap.h
    CMap3D.h
    IMapSelection.h
    CMapSelectionRaster.h
    CMapSelectionGarmin.h
    CMapSearchWidget.h
    CMapSearchCanvas.h
    CMapSearchThread.h
    CMegaMenu.h
    IDB.h
    CWpt.h
    CWptDB.h
    CWptToolWidget.h
    CSearch.h
    CSearchDB.h
    CSearchToolWidget.h
    CDlgConfig.h
    CDlgEditWpt.h
    CDlgLoadOnlineMap.h
    CDlgWptIcon.h
    CDlgEditMapLevel.h
    CDlgCombineTracks.h
    CDlgCombineDistOvl.h
    CDlgProjWizzard.h
    CDlgEditText.h
    CDlgDelWpt.h
    CDlgConvertToTrack.h
    CDlgEditRoute.h
    CDlgMapTDBConfig.h
    CDlgMapQMAPConfig.h
    CDlgMapJNXConfig.h
    CDlgMapTmsConfig.h
    CDlgMapWmsConfig.h
    CDlgMapRMPConfig.h
    CDlgNoMapConfig.h
    CDlgScreenshot.h
    CDlgTrackFilter.h
    CDlgSetupGarminIcons.h
    CDlgConfig3D.h
    CDlgExport.h
    CDlgWpt2Rte.h
    CDlgSetupGrid.h
    CDlgCropMap.h
    CDlgParentWpt.h
    CDlgDeviceExportPath.h
    CDlgProxy.h
    CDlgMultiColorConfig.h
    CDlg3DHelp.h
    CQlb.h
    CGpx.h
    IDevice.h
    CDeviceQLandkarteM.h
    CDeviceMagellan.h
    CDeviceGarmin.h
    CDeviceGarminBulk.h
    CDeviceNMEA.h
    CDeviceTwoNav.h
    CCreateMapQMAP.h
    CCreateMapGeoTiff.h
    CCreateMapGridTool.h
    CCreateMapFineTune.h
    CMapDB.h
    CMapToolWidget.h
    CMapEditWidget.h
    CUndoStackModel.h
    CUndoStackView.h
    CTrackUndoCommandSelect.h
    CTrack.h
    CTrackDB.h
    CTrackToolWidget.h
    CTrackEditWidget.h
    ITrackStat.h
    CTrackStatProfileWidget.h
    CTrackStatSpeedWidget.h
    CTrackStatTraineeWidget.h
    CTrackStatDistanceWidget.h
    CTrackFilterWidget.h
    CRoute.h
    CRouteDB.h
    CRouteToolWidget.h
    CTabWidget.h
    CDiary.h
    CDiaryDB.h
    CDiaryEdit.h
    CTextEditWidget.h
    CPlot.h
    CPlotAxis.h
    CPlotAxisTime.h
    CPlotData.h
    CLiveLog.h
    CLiveLogDB.h
    CLiveLogToolWidget.h
    IOverlay.h
    COverlayDB.h
    COverlayToolWidget.h
    COverlayText.h
    COverlayTextBox.h
    COverlayDistance.h
    COverlayDistanceEditWidget.h
    COverlayArea.h
    COverlayAreaEditWidget.h
    IUnit.h
    CUnitMetric.h
    CUnitImperial.h
    CUnitNautic.h
    CImage.h
    IGarminStrTbl.h
    CGarminStrTbl6.h
    CGarminStrTbl8.h
    CGarminStrTblUtf8.h
    CGarminPolygon.h
    CGarminPoint.h
    CGarminTile.h
    Garmin.h
    CGarminExport.h
    IGarminTyp.h
    CGarminTyp.h
    CGarminTypNT.h
    CMapTDB.h
    CAppOpts.h
    CHelpButton.h
    CSpline.h
    IItem.h
    CImageSelect.h
    CGridDB.h
    IDiskCache.h
    CDiskCache.h
    CDiskCacheZip.h
    CSettings.h
    CTextBrowser.h
    CImageViewer.h
    CMapRmp.h
    CMapDEMSlopeSetup.h
    QFileExt.h
    IExchange.h
    CExchangeGarmin.h
    CExchangeTwoNav.h
)


SET(qlandkarte_UIS
    ICopyright.ui
    IWptToolWidget.ui
    IMapToolWidget.ui
    IMapEditWidget.ui
    IMapSearchWidget.ui
    IMapQMAPExport.ui
    ITrackToolWidget.ui
    ITrackEditWidget.ui
    ITrackStatWidget.ui
    ITrackFilterWidget.ui
    ISearchToolWidget.ui
    IDlgConfig.ui
    IDlgEditWpt.ui
    IDlgWptIcon.ui
    IDlgEditMapLevel.ui
    IDlgCombineTracks.ui
    IDlgCombineDistOvl.ui
    IDlgDelWpt.ui
    IDlgConvertToTrack.ui
    IDlgEditRoute.ui
    IDlgLoadOnlineMap.ui
    IDlgMapTDBConfig.ui
    IDlgMapQMAPConfig.ui
    IDlgMapJNXConfig.ui
    IDlgMapTmsConfig.ui
    IDlgMapWmsConfig.ui
    IDlgMapRMPConfig.ui
    IDlgNoMapConfig.ui
    IDlgScreenshot.ui
    IDlgTrackFilter.ui
    IDlgSetupGarminIcons.ui
    IDlgConfig3D.ui
    IDlgExport.ui
    IDlgWpt2Rte.ui
    IDlgSetupGrid.ui
    IDlgCropMap.ui
    IDlgParentWpt.ui
    IDlgDeviceExportPath.ui
    IDlgProxy.ui
    IDlgMultiColorConfig.ui
    IDlg3DHelp.ui
    ICreateMapQMAP.ui
    ICreateMapGeoTiff.ui
    ICreateMapGridTool.ui
    ICreateMapFineTune.ui
    IRouteToolWidget.ui
    IStatusDEM.ui
    IDlgProjWizzard.ui
    ITextEditWidget.ui
    ILiveLogToolWidget.ui
    IOverlayToolWidget.ui
    IOverlayDistanceEditWidget.ui
    IOverlayAreaEditWidget.ui
    IGarminExport.ui
    IHelpDlg.ui
    IDiaryEdit.ui
    IImageSelect.ui
    IImageViewer.ui
    IMapDEMSlopeSetup.ui
)

SET(qlandkarte_RCS
    resources.qrc
)

add_definitions(-DDB_VERSION=9)
SET(qlandkarte_HDRS ${qlandkarte_HDRS}  CGeoDB.h CDlgSelGeoDBFolder.h CDlgEditFolder.h)
SET(qlandkarte_SRCS ${qlandkarte_SRCS}  CGeoDB.cpp CDlgSelGeoDBFolder.cpp CDlgEditFolder.cpp)
SET(qlandkarte_UIS ${qlandkarte_UIS} IGeoToolWidget.ui IDlgSelGeoDBFolder.ui IDlgEditFolder.ui)

SET (GPX_EXTENSIONS ON CACHE BOOL "Read <extensions> data from tracks in GPX files.")
if (NOT WIN32 AND NOT APPLE)
SET (DBUS ON CACHE BOOL "Enable DBus control.")
else (NOT WIN32 AND NOT APPLE)
SET (DBUS OFF CACHE BOOL "Enable DBus control.")
endif (NOT WIN32 AND NOT APPLE)
SET (ALTKEY_MOVES_MAP OFF CACHE BOOL "Move map while pressing ALT key and mousemove.")


if(ALTKEY_MOVES_MAP)
    add_definitions(-DALTKEY_MOVES_MAP)
endif(ALTKEY_MOVES_MAP)


if(GPX_EXTENSIONS)
    add_definitions(-DGPX_EXTENSIONS)
    SET(qlandkarte_HDRS ${qlandkarte_HDRS}  CTrackStatExtensionWidget.h CGpxExtension.h)
    SET(qlandkarte_SRCS ${qlandkarte_SRCS}  CTrackStatExtensionWidget.cpp CGpxExtension.cpp)
    SET(qlandkarte_UIS ${qlandkarte_UIS} )
endif(GPX_EXTENSIONS)

if(GPSD_FOUND)
    add_definitions(-DHAS_GPSD)
    SET(qlandkarte_HDRS ${qlandkarte_HDRS} CDeviceGPSD.h )
    SET(qlandkarte_SRCS ${qlandkarte_SRCS} CDeviceGPSD.cpp)
else(GPSD_FOUND)
    SET(GPSD_INCLUDE_DIRS "")
    SET(GPSD_LIBRARIES "")
endif(GPSD_FOUND)

IF(EXIF_FOUND)
    add_definitions(-DHAS_EXIF)
    SET(qlandkarte_HDRS ${qlandkarte_HDRS} CDlgImportImages.h )
    SET(qlandkarte_SRCS ${qlandkarte_SRCS} CDlgImportImages.cpp)
    SET(qlandkarte_UIS ${qlandkarte_UIS} IDlgImportImages.ui)
else(EXIF_FOUND)
    set(EXIF_INCLUDE_DIRS "")
    set(EXIF_LIBRARIES "")
ENDIF(EXIF_FOUND)

IF(DMTX_FOUND)
    add_definitions(-DHAS_DMTX)
else(DMTX_FOUND)
    set(DMTX_INCLUDE_DIRS "")
    set(DMTX_LIBRARIES "")
ENDIF(DMTX_FOUND)

IF(NOT QTSOAP_FOUND)
    set(QTSOAP_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/3rdparty/qtsoap")
    set(QTSOAP_LIBRARIES "qtsoap")
ENDIF()


IF(DBUS)
    add_definitions(-DHAS_DBUS)
    SET(qlandkarte_HDRS ${qlandkarte_HDRS}  CDBus.h)
    SET(qlandkarte_SRCS ${qlandkarte_SRCS}  CDBus.cpp)
    SET(qlandkarte_UIS ${qlandkarte_UIS})
ENDIF(DBUS)



if(QK_QT5_PORT)
  qt5_wrap_cpp(qlandkarte_MOC_SRCS ${qlandkarte_HDRS})
  qt5_wrap_ui(qlandkarte_UI_HDRS ${qlandkarte_UIS})
  qt5_add_resources(qlandkarte_RC_SRCS ${qlandkarte_RCS})
else(QK_QT5_PORT)
  qt4_wrap_cpp(qlandkarte_MOC_SRCS ${qlandkarte_HDRS})
  qt4_wrap_ui(qlandkarte_UI_HDRS ${qlandkarte_UIS})
  qt4_add_resources(qlandkarte_RC_SRCS ${qlandkarte_RCS})
endif(QK_QT5_PORT)

SET (ENABLE_TRANSLATION ON CACHE BOOL "enable translations")
#-D ENABLE_TRANSLATION:bool=OFF
IF(ENABLE_TRANSLATION)
    if(QK_QT5_PORT)
      find_package(Qt5LinguistTools)
    endif(QK_QT5_PORT)
    add_definitions(-DENABLE_TRANSLATION)

    file (GLOB TRANSLATIONS_FILES locale/*.ts)

    option (UPDATE_TRANSLATIONS "Update source translation translations/*.ts
    files (WARNING: make clean will delete the source .ts files! Danger!)")
    if (UPDATE_TRANSLATIONS)
       if(QK_QT5_PORT)
     qt5_create_translation(qlandkarte_QM_FILES
           ${qlandkarte_SRCS}
           ${qlandkarte_HDRS}
           ${qlandkarte_UIS}
           ${TRANSLATIONS_FILES}
           )
       else(QK_QT5_PORT)
     qt4_create_translation(qlandkarte_QM_FILES
           ${qlandkarte_SRCS}
           ${qlandkarte_HDRS}
           ${qlandkarte_UIS}
           ${TRANSLATIONS_FILES}
           )
       endif(QK_QT5_PORT)
    else (UPDATE_TRANSLATIONS)
      if (QK_QT5_PORT)
    qt5_add_translation(qlandkarte_QM_FILES ${TRANSLATIONS_FILES})
      else(QK_QT5_PORT)
    qt4_add_translation(qlandkarte_QM_FILES ${TRANSLATIONS_FILES})
      endif(QK_QT5_PORT)
    endif (UPDATE_TRANSLATIONS)

    if (UNIX AND NOT WIN32 AND NOT APPLE)
     install(
             FILES
                   ${qlandkarte_QM_FILES}
             DESTINATION
                   ${DATA_INSTALL_PREFIX}/qlandkartegt/translations)
    endif (UNIX AND NOT WIN32 AND NOT APPLE)
    if (APPLE)
      add_custom_target(locversion.plist ALL
        COMMAND
            ${CMAKE_SOURCE_DIR}/MacOSX/mklocversion.sh ${MAC_BUNDLE_PATH} ${qlandkarte_QM_FILES}
      )
    endif (APPLE)
ENDIF(ENABLE_TRANSLATION)

if(UNIX)
    add_definitions(-D_TTY_POSIX_ -Wall)
endif(UNIX)


set(qlandkarte_include_dirs
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${GDAL_INCLUDE_DIRS}
  ${PROJ_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/3rdparty/QZip
  ${CMAKE_SOURCE_DIR}/3rdparty/QTextHtmlExporter
  ${CMAKE_SOURCE_DIR}/3rdparty/QDeviceWatcher
  ${CMAKE_SOURCE_DIR}/3rdparty/getopt
  ${QTSOAP_INCLUDE_DIRS}
  ${EXIF_INCLUDE_DIRS}
  ${DMTX_INCLUDE_DIRS}
  ${GPSD_INCLUDE_DIRS}
  ${QEXTSERIALPORT_INCLUDE_DIRS}
)

if (NOT QK_QT5_PORT)
set(qlandkarte_include_dirs ${qlandkarte_include_dirs}
  ${QT_QTXML_INCLUDE_DIR}
  ${QT_QTSQL_INCLUDE_DIR}
  ${QT_QTOPENGL_INCLUDE_DIR}
  ${QT_QTNETWORK_INCLUDE_DIR}
  ${QT_QTDBUS_INCLUDE_DIR}
  ${QT_QTWEBKIT_INCLUDE_DIR}
  ${QT_QTSCRIPT_INCLUDE_DIR}
  ${QT_OPENGL_INCLUDE_DIR}
)
endif(NOT QK_QT5_PORT)


if(NOT QK_QT5_SERIAL_PORT)
  set(qlandkarte_include_dirs
    ${qlandkarte_include_dirs}
    ${CMAKE_SOURCE_DIR}/3rdparty/SerialPort
  )
endif(NOT QK_QT5_SERIAL_PORT)

if(NOT QK_QT5_TZONE)
  set(qlandkarte_include_dirs
    ${qlandkarte_include_dirs}
    ${CMAKE_SOURCE_DIR}/3rdparty/tzone/include
  )
endif(NOT QK_QT5_TZONE)

include_directories(
  ${qlandkarte_include_dirs}
)

set(qlandkarte_ALLINP
    ${qlandkarte_SRCS}
    ${qlandkarte_HDRS}
    ${qlandkarte_UI_HDRS}
    ${qlandkarte_RC_SRCS}
    ${qlandkarte_MOC_SRCS}
    ${qlandkarte_QM_FILES}
)

if (APPLE)
  set(MAC_EXTRA_RES
      ${CMAKE_SOURCE_DIR}/MacOSX/${MAC_ICON_FILE}
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-gdb.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-gpx.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-kml.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-loc.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-plt.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-qlb.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-rte.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-tcx.icns
      ${CMAKE_SOURCE_DIR}/MacOSX/qlandkartegt-wpt.icns
  )
  set_source_files_properties(
    ${MAC_EXTRA_RES} ${qlandkarte_QM_FILES}
  PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
  set(MACOSX_BUNDLE_COPYRIGHT
    "Copyright © 2007-2011 Oliver Eichler (oliver.eichler@gmx.de)"
  )
  add_executable(${PROGNAME} MACOSX_BUNDLE ${qlandkarte_ALLINP} ${MAC_EXTRA_RES})
  set_target_properties(${PROGNAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/MacOSX/Info.plist.in
        LINK_FLAGS "-framework CoreFoundation -framework DiskArbitration"
  )

  add_custom_target(mac-deploy macdeployqt "${MAC_BUNDLE_PATH}"
    COMMAND ${CMAKE_SOURCE_DIR}/MacOSX/map2gcm-libs.sh "${MAC_BUNDLE_PATH}")
else(APPLE)
  add_executable(${PROGNAME} WIN32 ${qlandkarte_ALLINP})
endif(APPLE)

if(QK_QT5_PORT)
  qt5_use_modules(${PROGNAME} Widgets)
  qt5_use_modules(${PROGNAME} Quick)
  qt5_use_modules(${PROGNAME} Network)
  qt5_use_modules(${PROGNAME} Gui)
  qt5_use_modules(${PROGNAME} Xml)
  qt5_use_modules(${PROGNAME} Sql)
  qt5_use_modules(${PROGNAME} OpenGL)
  if (NOT APPLE)
    qt5_use_modules(${PROGNAME} DBus)
  endif(NOT APPLE)
  qt5_use_modules(${PROGNAME} PrintSupport)
  qt5_use_modules(${PROGNAME} Script)
  qt5_use_modules(${PROGNAME} Multimedia)
  qt5_use_modules(${PROGNAME} WebKitWidgets)
  qt5_use_modules(${PROGNAME} SerialPort)
endif(QK_QT5_PORT)


if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE )
endif(MSVC)

set(qlandkarte_link_libs
    ${PROGNAME}
    ${OPENGL_LIBRARIES}
    ${GDAL_LIBRARIES}
    ${PROJ_LIBRARIES}
    ${QT_LIBRARIES}
    ${QT_QTNETWORK_LIBRARIES}
    ${QT_QTXML_LIBRARIES}
    ${QT_QTSQL_LIBRARIES}
    ${QT_QTDBUS_LIBRARIES}
    ${QT_QTMAIN_LIBRARIES}
    ${QT_QTWEBKIT_LIBRARIES}
    ${QT_PHONON_LIBRARIES}
    ${QT_QTSCRIPT_LIBRARIES}
    ${QT_QTXMLPATTERNS_LIBRARIES}
    ${QT_QTOPENGL_LIBRARIES}
    ${EXIF_LIBRARIES}
    ${DMTX_LIBRARIES}
    ${GPSD_LIBRARIES}
    qzip
    qtexthtmlexporter
    getopt
    ${QTSOAP_LIBRARIES}
    ${PROFILING_LIBRARIES}
)

if(NOT QK_QT5_SERIAL_PORT)
  set(qlandkarte_link_libs
    ${qlandkarte_link_libs}
    SerialPort
  )
endif(NOT QK_QT5_SERIAL_PORT)

if(NOT QK_QT5_TZONE)
  set(qlandkarte_link_libs
    ${qlandkarte_link_libs}
    tzone
  )
endif(NOT QK_QT5_TZONE)

target_link_libraries(
  ${qlandkarte_link_libs}
)

install(
    TARGETS
      ${PROGNAME}
    DESTINATION
      ${BIN_INSTALL_DIR}
)
