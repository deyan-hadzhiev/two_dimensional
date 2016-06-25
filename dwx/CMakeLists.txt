
if ("${WX_WIDGETS_PATH}" STREQUAL "")
	message(FATAL_ERROR "WX_WIDGETS_PATH variable is not set!")
elseif (NOT EXISTS ${WX_WIDGETS_PATH})
	message(FATAL_ERROR "Path to wxWidgets external location is required and must exist! ${WX_WIDGETS_PATH}")
endif()

set(PROJECT_NAME dwx)
project(${PROJECT_NAME})

add_definitions(
	-DUNICODE
	-D_UNICODE
)

set (PUBLIC_HEADERS
	include/
	${WX_WIDGETS_PATH}/include
	${WX_WIDGETS_PATH}/include/msvc
)

set (HEADERS
	include/bitmap.h
	include/color.h
	include/constants.h
	include/convolution.h
	include/drect.h
	include/fft_butterfly.h
	include/geom_primitive.h
	include/guimain.h
	include/matrix2.h
	include/modules.h
	include/module_base.h
	inclide/module_manager.h
	include/util.h
	include/vector2.h
	include/wx_bitmap_canvas.h
	include/wx_modes.h
	include/wx_param_panel.h
)

set (SOURCES
	src/bitmap.cpp
	src/color.cpp
	src/convolution.cpp
	src/fft_butterfly.cpp
	src/geom_primitive.cpp
	src/guimain.cpp
	src/matrix2.cpp
	src/modules.cpp
	src/module_manager.cpp
	src/util.cpp
	src/wx_modes.cpp
	src/wx_bitmap_canvas.cpp
	src/wx_param_panel.cpp
)

link_directories (
	${WX_WIDGETS_PATH}/lib/vc_x64_lib
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})

ir_add_install ("${PROJECT_NAME}")
