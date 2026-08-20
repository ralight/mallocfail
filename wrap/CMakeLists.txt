add_library(wrap STATIC
	../include/mallocfail/wrap.h
	mallocfailwrap.c
)
set_target_properties(wrap PROPERTIES
	OUTPUT_NAME mallocfailwrap
	POSITION_INDEPENDENT_CODE 1
)
target_include_directories(wrap PRIVATE
	${mallocfail_SOURCE_DIR}/include
)

install(TARGETS wrap
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

target_link_options(wrap INTERFACE
	"LINKER:--wrap=malloc"
	"LINKER:--wrap=calloc"
	"LINKER:--wrap=realloc"
	"LINKER:--wrap=strdup"
)
target_link_libraries(wrap INTERFACE
	backtrace
)

target_include_directories(wrap PUBLIC
	$<BUILD_INTERFACE:${mallocfail_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_compile_definitions(wrap INTERFACE
	MALLOCFAILWRAP
)

add_executable(mf_test_wrap ../mf_test.c)
target_link_libraries(mf_test_wrap wrap)

install(
	TARGETS wrap
	EXPORT mallocfailTargets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
)
install(
	DIRECTORY ${mallocfail_SOURCE_DIR}/include/
	DESTINATION include
)
