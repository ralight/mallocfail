cmake_minimum_required(VERSION 4.0)

set(VERSION 0.1)

project(mallocfail
	LANGUAGES C
	VERSION ${VERSION}
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)
list(APPEND CMAKE_MODULE_PATH "${mallocfail_SOURCE_DIR}/cmake/")

add_subdirectory(ld_preload)
add_subdirectory(wrap)

install(
	EXPORT mallocfailTargets
	FILE mallocfailTargets.cmake
	NAMESPACE mallocfail::
	DESTINATION lib/cmake/mallocfail
)

configure_package_config_file(
	${mallocfail_SOURCE_DIR}/cmake/mallocfailConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mallocfailConfig.cmake
	INSTALL_DESTINATION lib/cmake/mallocfail
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/mallocfailConfigVersion.cmake"
	COMPATIBILITY SameMajorVersion
)
install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/mallocfailConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/mallocfailConfigVersion.cmake"
	DESTINATION lib/cmake/mallocfail
)
