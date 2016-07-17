
if ("${WX_WIDGETS_PATH}" STREQUAL "")
	message(FATAL_ERROR "WX_WIDGETS_PATH variable is not set!")
elseif (NOT EXISTS ${WX_WIDGETS_PATH})
	message(FATAL_ERROR "Path to wxWidgets external location is required and must exist! ${WX_WIDGETS_PATH}")
endif()

set(PROJECT_NAME png2header)
project(${PROJECT_NAME})

add_definitions(
	-DUNICODE
	-D_UNICODE
)

set (PUBLIC_HEADERS
	${WX_WIDGETS_PATH}/include
	${WX_WIDGETS_PATH}/include/msvc
)

set (SOURCES
	main.cpp
)

link_directories (
	${WX_WIDGETS_PATH}/lib/vc_x64_lib
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})

ir_add_install ("${PROJECT_NAME}")
