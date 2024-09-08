
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

add_executable (Tests "src/expect.h" "src/test_manager.h" "src/test_manager.cpp" "src/main.cpp" 
	"src/memory/linear_allocator_tests.h" "src/memory/linear_allocator_tests.cpp"
	"src/memory/dynamic_allocator_tests.h" "src/memory/dynamic_allocator_tests.cpp"
	"src/memory/stack_allocator_tests.h" "src/memory/stack_allocator_tests.cpp"
	"src/containers/array_tests.h" "src/containers/array_tests.cpp"
	"src/containers/hash_table_tests.h" "src/containers/hash_table_tests.cpp"
	"src/containers/hash_map_tests.h" "src/containers/hash_map_tests.cpp"
	"src/containers/dynamic_array_tests.h" "src/containers/dynamic_array_tests.cpp"
	"src/containers/stack_tests.h" "src/containers/stack_tests.cpp"
	"src/containers/ring_queue_tests.h" "src/containers/ring_queue_tests.cpp"
	"src/containers/queue_tests.h" "src/containers/queue_tests.cpp"
	"src/string/string_tests.h" "src/string/string_tests.cpp"
	"src/string/cstring_tests.h" "src/string/cstring_tests.cpp"
	"src/platform/file_system.h" "src/platform/file_system.cpp"
	"src/function/stack_function_tests.h" "src/function/stack_function_tests.cpp"
	"src/parsers/cson_parser_tests.h" "src/parsers/cson_parser_tests.cpp"
)

target_link_libraries(Tests PUBLIC C3DEngineCore)

add_custom_target(CopyDLLTests
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/engine.core/${CMAKE_SHARED_LIBRARY_PREFIX}C3DEngineCore${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/tests" DEPENDS C3DEngineCore
)

add_dependencies(Tests CopyDLLTests)