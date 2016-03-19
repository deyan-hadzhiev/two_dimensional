
cmake_minimum_required (VERSION 3.4)

# macro for installations
macro (ir_add_install _INSTALL_NAME)
	if (EXISTS ${INSTALL_DIR})
		message (" >> Installing ${_INSTALL_NAME} to ${INSTALL_DIR}}")
		install(TARGETS ${_INSTALL_NAME} DESTINATION ${INSTALL_DIR} CONFIGURATIONS Debug Release RelWithDebInfo)
	else()
		message (" >! No install dir defined for ${_INSTALL_NAME}")
	endif()
endmacro()

project(two_dimensional)

if (NOT EXISTS ${SDL_PATH})
	message(FATAL_ERROR "Path to sdl external location is required and must exist!")
endif()

add_subdirectory(dsdl)

if (EXISTS ${INSTALL_DIR})
	install(FILES ${SDL_PATH}/lib/x64/SDL2.dll DESTINATION ${INSTALL_DIR})
endif()
