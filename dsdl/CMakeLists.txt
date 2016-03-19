
set(PROJECT_NAME dsdl)
project(${PROJECT_NAME})

set (PUBLIC_HEADERS
	include/
	${SDL_PATH}/include
)

set (HEADERS
	include/charstring.h
	include/error.h
	include/render.h
	include/texture.h
	include/utils.h
)

set (SOURCES
	src/charstring.cpp
	src/render.cpp
	src/texture.cpp
)

link_directories (
	${SDL_PATH}/lib/x64
)

set (PROJECT_TEST ${PROJECT_NAME}_test)
add_executable(${PROJECT_TEST} WIN32 ${SOURCES} src/main.cpp ${HEADERS})
target_include_directories(${PROJECT_TEST} PUBLIC ${PUBLIC_HEADERS})
target_link_libraries (${PROJECT_TEST} SDL2)

ir_add_install ("${PROJECT_TEST}")

#if (EXISTS ${INSTALL_DIR})
#	install(TARGETS ${PROJECT_TEST} DESTINATION ${INSTALL_DIR})
#endif()

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})
target_link_libraries (${PROJECT_NAME} SDL2)
