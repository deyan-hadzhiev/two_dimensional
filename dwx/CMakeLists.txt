
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
	include/matrix2.h
	include/transform.h
	include/vector2.h
	include/drect.h
	include/util.h
	include/guimain.h
	include/wx_modes.h
	include/wx_bitmap_canvas.h
	include/wx_param_panel.h
	include/module_base.h
	include/modules.h
	include/geom_primitive.h
)

set (SOURCES
	src/util.cpp
	src/convolution.cpp
	src/guimain.cpp
	src/wx_modes.cpp
	src/wx_bitmap_canvas.cpp
	src/wx_param_panel.cpp
	src/bitmap.cpp
	src/color.cpp
	src/matrix2.cpp
	src/modules.cpp
	src/geom_primitive.cpp
)

link_directories (
	${WX_WIDGETS_PATH}/lib/vc_x64_lib
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})

ir_add_install ("${PROJECT_NAME}")
