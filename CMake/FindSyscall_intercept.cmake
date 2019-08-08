find_package(PkgConfig)
pkg_check_modules(PC_Syscall_intercept QUIET libsyscall_intercept)

find_path(Syscall_intercept_INCLUDE_DIR
  NAMES libsyscall_intercept_hook_point.h
  PATHS ${PC_Syscall_intercept_INCLUDE_DIRS}
)

find_library(Syscall_intercept_LIBRARY
  NAMES syscall_intercept
  PATHS ${PC_Syscall_intercept_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	Syscall_intercept
	DEFAULT_MSG
	Syscall_intercept_INCLUDE_DIR
	Syscall_intercept_LIBRARY
)

if(Syscall_intercept_FOUND)
  set(Syscall_intercept_LIBRARIES ${Syscall_intercept_LIBRARY})
  set(Syscall_intercept_INCLUDE_DIRS ${Syscall_intercept_INCLUDE_DIR})
  set(Syscall_intercept_DEFINITIONS ${PC_Syscall_intercept_CFLAGS_OTHER})

  if(NOT TARGET Syscall_intercept::Syscall_intercept)
	  add_library(Syscall_intercept::Syscall_intercept UNKNOWN IMPORTED)
	  set_target_properties(Syscall_intercept::Syscall_intercept PROPERTIES
		IMPORTED_LOCATION "${Syscall_intercept_LIBRARY}"
		INTERFACE_COMPILE_OPTIONS "${PC_Syscall_intercept_CFLAGS_OTHER}"
		INTERFACE_INCLUDE_DIRECTORIES "${Syscall_intercept_INCLUDE_DIR}"
	  )
	endif()
endif()


mark_as_advanced(
	Syscall_intercept_INCLUDE_DIR
	Syscall_intercept_LIBRARY
)
