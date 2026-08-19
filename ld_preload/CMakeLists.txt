add_library(preload SHARED
	mallocfail_preload.h
	mallocfail_preload.c
	memory_funcs.c
	${mallocfail_SOURCE_DIR}/deps/sha3/sha3.c
)
set_target_properties(preload PROPERTIES
	OUTPUT_NAME mallocfailpreload
)

target_include_directories(preload PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${mallocfail_SOURCE_DIR}/deps/sha3
	${mallocfail_SOURCE_DIR}/deps/uthash
)

target_link_libraries(preload INTERFACE
	backtrace
)

set_target_properties(preload PROPERTIES
	PREFIX ""
)

add_executable(mf_test_preload ../mf_test.c)
target_link_libraries(mf_test_preload preload)

install(
	TARGETS preload
	EXPORT mallocfailTargets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
)

configure_file(
	mallocfailpreload.in
	mallocfailpreload
	@ONLY
)

install(
	PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mallocfailpreload
	DESTINATION bin
)
