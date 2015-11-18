cmake_minimum_required( VERSION 2.8.11 )
set (CMAKE_BUILD_TYPE Release )
set (objectName sdr-j-dabreceiver-0.99)
set ( CMAKE_INSTALL_PREFIX . )
add_definitions ( -Wall)
######################################################################
#       R E A D  T H I S
#####################################################################
#	adjust to your likings
#	if you want to use the SSE support on your machine, uncomment
#	set(NO_SSE_SUPPORT true)
#
#	if you want support for any of these devices, uncomment the line
	set(SDRPLAY true)
	set(AIRSPY true)
	set(DABSTICK true)
#	set(RTLTCP true)
	set(SPECTRUM true)

########################################################################
	find_package (Qt5Widgets REQUIRED)
	find_package (Qt5Declarative REQUIRED)

	find_library (DYNAMIC_LOAD dl)
	if(NOT(DYNAMIC_LOAD))
	   message(FATAL_ERROR "please install -ldl")
	else(NOT(DYNAMIC_LOAD))
	   set(extraLibs ${DYNAMIC_LOAD})
	endif(NOT(DYNAMIC_LOAD))
#

#	find_library(QWT qwt)
	if (QWT_INCLUDE_DIRS)
#	Already in cache, be silent
	   set (QWT_FIND_QUIETLY TRUE)
	endif (QWT_INCLUDE_DIRS)

	find_path (QWT_INCLUDE_DIR
	           NAMES qwt_plot.h
	           PATHS
	           /usr/local/include/qwt-qt5
	           /usr/local/include/qwt
	           /usr/local/include/qwt6
	           /usr/include/qwt6
	           /usr/include/qwt
	)

	list (APPEND QWT_INCLUDE_DIRS ${QWT_INCLUDE_DIR} )
	find_library (QWT_LIBRARIES
	              NAMES qwt-qt5 qwt qwt6
#	              PATHS /usr/local/lib /usr/lib
                      PATH_SUFFIXES lib64 Lib64 lib Lib
	)

#	message (STATUS ${QWT_LIBRARIES} ${QWT_INCLUDE_DIRS})
#	handle the QUIETLY and REQUIRED arguments and set QWT_FOUND to TRUE if 
#	all listed variables are TRUE
	include (FindPackageHandleStandardArgs)
	find_package_handle_standard_args (
	                     Qwt DEFAULT_MSG QWT_LIBRARIES QWT_INCLUDE_DIRS )
	MARK_AS_ADVANCED (QWT_LIBRARIES QWT_INCLUDE_DIRS)
#	Find_Package(PkgConfig)
#	pkg_check_modules(FFTW fftw3f REQUIRED)
#	link_directories(${FFTW_INCLUDE_DIRS})
#	Find the native FFTW includes and library
#
#	FFTW_INCLUDES    - where to find fftw3f.h
#	FFTW_LIBRARIES   - List of libraries when using FFTW.
#	FFTW_FOUND       - True if FFTW found.

	if (FFTW_INCLUDE_DIRS)
#	Already in cache, be silent
	   set (FFTW_FIND_QUIETLY TRUE)
	endif (FFTW_INCLUDE_DIRS)

	find_path (FFTW_INCLUDE_DIR fftw3.h
	           HINTS
                   ENV FFTW_ROOT
                   PATHS 
	           PATH_SUFFIXES
	           include
	           Include
	)

	list (APPEND FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR})
	find_library (FFTW_LIBRARIES NAMES fftw3f
                      HINTS ENV FFTW_ROOT
                      PATHS
                      PATH_SUFFIXES lib64 Lib64 lib Lib)

	message (STATUS ${FFTW_LIBRARIES} ${FFTW_INCLUDE_DIRS})
#	handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
#	all listed variables are TRUE

	include (FindPackageHandleStandardArgs)
	find_package_handle_standard_args (
	                  FFTW DEFAULT_MSG FFTW_LIBRARIES FFTW_INCLUDE_DIRS)
	mark_as_advanced (FFTW_LIBRARIES FFTW_INCLUDE_DIRS)

	Find_Package (PkgConfig)
	find_library (FAAD faad)
	if (NOT(FAAD))
	   message(FATAL_ERROR "please install libfaad")
	else (NOT(FAAD))
	set (extraLibs  ${extraLibs} ${FAAD})
	endif(NOT(FAAD))

	find_library (PORTAUDIO portaudio)
	if (NOT(PORTAUDIO))
	   message(FATAL_ERROR "please install portaudio V19")
	else(NOT(PORTAUDIO))
	   set (extraLibs  ${extraLibs} ${PORTAUDIO})
	endif(NOT(PORTAUDIO))

	SET (CMAKE_FIND_LIBRARY_SUFFIXES .so .a)

	find_library (USB1 usb-1.0 HINTS /usr/lib)
	if (NOT(USB1))
	   message (FATAL_ERROR "please install libusb-1.0")
	else(NOT(USB1))
	   set(extraLibs ${extraLibs} ${USB1})
	endif(NOT(USB1))

	find_library (SNDFILE sndfile)
	if (NOT(SNDFILE))
	   message (FATAL_ERROR "please install libsndfile")
	else (NOT(SNDFILE))
	   set (extraLibs ${extraLibs} ${SNDFILE})
	endif (NOT(SNDFILE))

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))
#######################################################################
#
#	Here we really start
	set ( ${objectName}_UIS ./sdrgui.ui)

	if (NO_SSE_SUPPORT) 
	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/backend/spiral-code/spiral-no-sse.c )
	else ()
	   add_definitions (-DSSE_AVAILABLE)
	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/backend/spiral-code/spiral-sse.c )
	endif()

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           ${QT5Widgets_INCLUDES}
	           ${QT_QTCORE_INCLUDE_DIR}
	           ${QT_QTGUI_INCLUDE_DIR}
	           .
	           ./
	           ./includes
	           ./includes/ofdm
	           ./includes/backend
	           ./includes/various
	           ./includes/output
	           ./includes/scopes-qwt6
	           ./src/input
	           ./src/input/rawfiles
	           ./src/input/wavfiles
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ./gui.h
	     ./includes/dab-constants.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/phasereference.h
	     ./includes/ofdm/phasetable.h
	     ./includes/ofdm/mapper.h
	     ./includes/backend/viterbi.h
	     ./includes/backend/msc-handler.h
	     ./includes/backend/rscodec.h
	     ./includes/backend/deconvolve.h
	     ./includes/backend/firecode-checker.h
	     ./includes/backend/dab-serial.h
	     ./includes/backend/dab-concurrent.h
	     ./includes/backend/dab-processor.h
	     ./includes/backend/dab-virtual.h
	     ./includes/backend/mp2processor.h
	     ./includes/backend/faad_decoder.h
	     ./includes/backend/charsets.h
	     ./src/input/virtual-input.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	     ./includes/output/fir-filters.h
	     ./includes/output/audiosink.h
	     ./includes/various/fft.h
	     ./includes/various/ringbuffer.h
	     ./includes/various/Xtan2.h
	     ./includes/scopes-qwt6/spectrogramdata.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./gui.cpp
	     ./src/ofdm/ofdm-processor.cpp
	     ./src/ofdm/ofdm-decoder.cpp
	     ./src/ofdm/phasereference.cpp
	     ./src/ofdm/phasetable.cpp
	     ./src/ofdm/mapper.cpp
	     ./src/backend/viterbi.cpp
	     ./src/backend/fic-handler.cpp
	     ./src/backend/msc-handler.cpp
	     ./src/backend/deconvolve.cpp
	     ./src/backend/fib-processor.cpp
	     ./src/backend/rscodec.cpp
	     ./src/backend/mp2processor.cpp
	     ./src/backend/mp4processor.cpp
	     ./src/backend/firecode-checker.cpp
	     ./src/backend/dab-serial.cpp
	     ./src/backend/dab-concurrent.cpp
	     ./src/backend/dab-virtual.cpp
	     ./src/backend/dab-processor.cpp
	     ./src/backend/protTables.cpp
	     ./src/backend/charsets.cpp
	     ./src/input/virtual-input.cpp
	     ./src/input/rawfiles/rawfiles.cpp
	     ./src/input/wavfiles/wavfiles.cpp
	     ./src/output/audiosink.cpp
	     ./src/output/fir-filters.cpp
	     ./src/scopes-qwt6/scope.cpp
	     ./src/scopes-qwt6/iqdisplay.cpp
	     ./src/various/fft.cpp
	     ./src/various/Xtan2.cpp
	)

	set (${objectName}_MOCS
	     ./gui.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/ofdm-decoder.h
	     ./includes/backend/fic-handler.h
	     ./includes/backend/fib-processor.h
	     ./includes/backend/mp2processor.h
	     ./includes/backend/mp4processor.h
	     ./includes/scopes-qwt6/scope.h
	     ./includes/scopes-qwt6/iqdisplay.h
	     ./src/input/rawfiles/rawfiles.h
	     ./src/input/wavfiles/wavfiles.h
	)

	if (SDRPLAY)
	   find_library (SDRPLAYLIB libmirsdrapi-rsp.so)
	   if (NOT(SDRPLAYLIB))
	      message (FATAL_ERROR "please install sdrplay library")
	   else (NOT(SDRPLAYLIB))
	      set (extraLibs ${extraLibs} ${SDRPLAYLIB})
	   endif()
	   find_path (SDRPLAYLIB_INCLUDE_DIR
	              NAMES mirsdrapi-rsp.h
	              PATHS
	              /usr/local/include/
	   )
	   include_directories (${SDRPLAYLIB_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/sdrplay/sdrplay-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/sdrplay/sdrplay.h
	        ./src/input/sdrplay/sdrplay-worker.h
	   )

	   include_directories (
	     ./src/input/sdrplay
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/sdrplay/sdrplay.h
	        ./src/input/sdrplay/sdrplay-loader.h
	        ./src/input/sdrplay/sdrplay-worker.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/sdrplay/sdrplay.cpp
	        ./src/input/sdrplay/sdrplay-worker.cpp
	        ./src/input/sdrplay/sdrplay-loader.cpp
	   )

	   add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)

	if (AIRSPY)
	   find_library (AIRSPY_LIB airspy)
	   if (NOT(AIRSPY_LIB))
	      message (FATAL_ERROR "please install airspy library")
	   else (NOT(AIRSPY_LIB))
	      set (extraLibs ${extraLibs} ${AIRSPY_LIB})
	   endif()
	   find_path (AIRSPYLIB_INCLUDE_DIR
	              NAMES airspy.h
	              PATHS
	              /usr/local/include/
	              /usr/local/include/libairspy
	   )
	   include_directories (${AIRSPYLIB_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/airspy/airspy-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/airspy/airspy-handler.h
	   )

	   include_directories (
	     ./src/input/airspy
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/airspy/airspy-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/airspy/airspy-handler.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)

	if (DABSTICK)
	   find_library (RTLSDR rtlsdr)
	   if (NOT(RTLSDR))
	      message (FATAL_ERROR "please install librtlsdr")
	   else (NOT(RTLSDR))
	      set (extraLibs ${extraLibs} ${RTLSDR})
	   endif()
	   find_path (DABSTICK_INCLUDE_DIR
	              NAMES rtl-sdr.h
	              PATHS
	              /usr/local/include/
	              /usr/include/
	   )
	   include_directories (${RTLSDR_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/dabstick/dabstick-widget.ui
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/dabstick/dongleselect.h
	        ./src/input/dabstick/dabstick.h
	   )

	   include_directories (
	        ./src/input/dabstick/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/dabstick/dabstick.h 
	        ./src/input/dabstick/dongleselect.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/dabstick/dabstick.cpp
	        ./src/input/dabstick/dongleselect.cpp
	   )

	   add_definitions (-DHAVE_DABSTICK)
	endif()
#
	if (RTLTCP)
	   find_package (Qt5Network REQUIRED)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/rtl_tcp/rtl_tcp-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/rtl_tcp/rtl_tcp_client.h
	   )

	   include_directories (
	      ./src/input/rtl_tcp
	      ${Qt5Network_INCLUDE_DIRS}
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/rtl_tcp/rtl_tcp_client.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/rtl_tcp/rtl_tcp_client.cpp
	   )

	   set (RTLTCP_lib Qt5::Network)
	   add_definitions (-DHAVE_RTL_TCP)
	endif (RTLTCP)

	if (SPECTRUM)
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./optional-scope/scopewidget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./optional-scope/spectrum-handler.h
	   )

	   include_directories (
	      ./optional-scope
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./optional-scope/spectrum-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./optional-scope/spectrum-handler.cpp
	   )

	   add_definitions (-DHAVE_SPECTRUM)
	endif (SPECTRUM)



	QT5_WRAP_UI (UIS ${${objectName}_UIS}
	             ./src/input/filereader-widget.ui )

	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${QT5Widgets_INCLUDES}
	          ${QT_QTCORE_INCLUDE_DIR}
	          ${QT_QTGUI_INCLUDE_DIR}
	          ${QWT_INCLUDE_DIRS}
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

	QT5_WRAP_CPP (MOCS ${${objectName}_MOCS})

	add_executable (${objectName}
	                ${${objectName}_SRCS}
	                ${UIS}
	                ${MOCS}
	)

	target_link_libraries (${objectName}
	                       Qt5::Widgets
	                       ${RTLTCP_lib}
	                       ${FFTW_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${QWT_LIBRARIES}
	)

	INSTALL (TARGETS ${objectName} DESTINATION ../linux-bin)
