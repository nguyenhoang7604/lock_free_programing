# include(GNUInstallDirs)

macro(init_project)
if(NOT CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-O -ggdb")
set(THREADS_PREFER_PTHREAD_FLAG ON)

endmacro(init_project)

option(UNIT_TESTING "Enable Unit Testing" ON)
option(ENABLE_TSAN "Enable Thread Sanitizer" ON)

macro(set_library_type lib)
	set(build_shared_var ${lib}_BUILD_SHARED)
	set(library_type_var ${lib}_LIBRARY_TYPE)
	if(${build_shared_var})
		set(${library_type_var} "SHARED" CACHE STRING "${lib} library type")
	else()
		set(${library_type_var} "STATIC" CACHE STRING "${lib} library type")
	endif()
endmacro(set_library_type)
