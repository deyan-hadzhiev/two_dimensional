if (WIN32)
	message(" >> Setting static runtime flags")
	set (CMAKE_C_FLAGS_DEBUG          "/MT /Zi /Od /Ob0 /D NDEBUG /D DDEBUG")
	set (CMAKE_C_FLAGS_MINSIZEREL     "/MT /Zi /01 /Ob1 /D NDEBUG")
	set (CMAKE_C_FLAGS_RELEASE        "/MT /Zi /Ox /Ob2 /Oi /Ot /GL /D NDEBUG")
	set (CMAKE_C_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob2 /D NDEBUG")

	set (CMAKE_CXX_FLAGS_DEBUG          "/MT /Zi /Od /Ob0 /D NDEBUG /D DDEBUG")
	set (CMAKE_CXX_FLAGS_MINSIZEREL     "/MT /Zi /01 /Ob1 /D NDEBUG")
	set (CMAKE_CXX_FLAGS_RELEASE        "/MT /Zi /Ox /Ob2 /Oi /Ot /GL /D NDEBUG")
	set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob2 /D NDEBUG")

	set (CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
endif()
