
if (NOT EXISTS ${WX_WIDGETS_PATH})
	message(FATAL_ERROR "Path to wxWidgets external location is required and must exist!")
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
	include/matrix2.h
	include/transform.h
	include/vector2.h
	include/util.h
	include/guimain.h
	include/wx_modes.h
	include/wx_bitmap_canvas.h
)

set (SOURCES
	src/guimain.cpp
	src/wx_bitmap_canvas.cpp
	src/wx_modes.cpp
	src/bitmap.cpp
	src/color.cpp
	src/matrix2.cpp
)

link_directories (
	${WX_WIDGETS_PATH}/lib/vc_x64_lib
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})

ir_add_install ("${PROJECT_NAME}")