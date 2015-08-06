rem Batch file to copy the necessary files for the Windows Installer
rem Please adapt environment variables in section 1) to your system

rem useful links
rem http://technet.microsoft.com/en-us/library/bb491035.aspx
rem http://vlaurie.com/computers2/Articles/environment.htm

rem Section 1.) Define path to Qt, MSVC, .... installations
set QLGTI_QT_PATH=C:\Qt\4.8.5
set QLGTI_VCREDIST_PATH="D:\qlgt\tools\vcredist_VS2008SP1"
set QLGTI_LIBEXIF_PATH="D:\qlgt\tools\libexif"
set QLGTI_GDAL_PATH="%CD%\..\Win32\gdal\bin"

rem Section 2.) Copy Files
del /s/q Files
mkdir Files
cd Files
rem Section 2.1) Copy Qt files
copy %QLGTI_QT_PATH%\bin\QtCore4.dll
copy %QLGTI_QT_PATH%\bin\QtGui4.dll
copy %QLGTI_QT_PATH%\bin\QtNetwork4.dll
copy %QLGTI_QT_PATH%\bin\QtOpenGL4.dll
copy %QLGTI_QT_PATH%\bin\QtSql4.dll
copy %QLGTI_QT_PATH%\bin\QtSvg4.dll
copy %QLGTI_QT_PATH%\bin\QtWebKit4.dll
copy %QLGTI_QT_PATH%\bin\phonon4.dll
copy %QLGTI_QT_PATH%\bin\QtXml4.dll
copy %QLGTI_QT_PATH%\bin\QtScript4.dll
mkdir imageformats
cd imageformats
copy %QLGTI_QT_PATH%\plugins\imageformats\qgif4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qjpeg4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qmng4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qsvg4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qtiff4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qico4.dll
copy %QLGTI_QT_PATH%\plugins\imageformats\qtga4.dll
cd ..
mkdir sqldrivers
cd sqldrivers
copy %QLGTI_QT_PATH%\plugins\sqldrivers\qsqlite4.dll
cd ..
rem  The qt_??.qm files must have been created before by
rem opening a qt shell, going to the translations directory and running
rem for %f in (qt_??.ts) do lrelease %f
copy %QLGTI_QT_PATH%\translations\qt_??.qm
rem section 2.2) Copy GDAL Files
mkdir gdal_bin
cd gdal_bin
xcopy %QLGTI_GDAL_PATH%\curl curl /s /i
xcopy %QLGTI_GDAL_PATH%\debug debug /s /i
mkdir gdal
cd gdal
xcopy %QLGTI_GDAL_PATH%\gdal\apps apps /s /i
xcopy %QLGTI_GDAL_PATH%\gdal\plugins plugins /s /i
xcopy %QLGTI_GDAL_PATH%\gdal\plugins-optional plugins-optional /s /i
xcopy %QLGTI_GDAL_PATH%\gdal\python python /s /i
cd ..
xcopy %QLGTI_GDAL_PATH%\gdal-data gdal-data /s /i
xcopy %QLGTI_GDAL_PATH%\proj proj /s /i
copy %QLGTI_GDAL_PATH%\*.dll
cd ..
rem section 2.3) Copy MSVC Redist Files
copy %QLGTI_VCREDIST_PATH%\vcredist_x86.exe
rem section 2.4) Copy libexif Files
copy %QLGTI_LIBEXIF_PATH%\libexif-12.dll
rem section 2.5) Copy QLandkarte GT Files
copy ..\..\build\bin\Release\qlandkartegt.exe
copy ..\..\build\bin\Release\map2gcm.exe
copy ..\..\build\bin\Release\map2rmap.exe
copy ..\..\build\bin\Release\cache2gtiff.exe
copy ..\..\build\bin\Release\map2rmp.exe
copy ..\..\build\bin\Release\map2jnx.exe
copy ..\..\build\src\*.qm
copy ..\..\src\icons\GlobeWin.ico
copy ..\*.ico
rem section 2.6) 3rd party SW description
copy ..\3rdparty.txt

pause