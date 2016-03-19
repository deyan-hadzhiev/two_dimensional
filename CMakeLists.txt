
cmake_minimum_required (VERSION 3.4)

include(flag_overrides.cmake)

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

#option (WTIH_SDL CACHE OFF STRING "Compile the dsdl project")
# TODO see how its done!
if (WITH_SDL)
	add_subdirectory(dsdl)
endif()

add_subdirectory(dwx)

if (EXISTS ${INSTALL_DIR})
	install(FILES ${SDL_PATH}/lib/x64/SDL2.dll DESTINATION ${INSTALL_DIR})
endif()
