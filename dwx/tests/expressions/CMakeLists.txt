
set(PROJECT_NAME expressions_test)
project(${PROJECT_NAME})

add_definitions(
	-DUNICODE
	-D_UNICODE
)

set (PUBLIC_HEADERS
	../../include/
)

set (HEADERS
	../../include/arithmetic.h
	../../include/constants.h
	../../include/dcomplex.h
	../../include/util.h
)

set (SOURCES
	../../src/arithmetic.cpp
	../../src/util.cpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_HEADERS})

ir_add_install ("${PROJECT_NAME}")
