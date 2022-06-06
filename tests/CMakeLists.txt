
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

add_executable (Tests "src/expect.h" "src/test_manager.h" "src/test_manager.cpp" "src/main.cpp" 
	"src/memory/linear_allocator_tests.h" "src/memory/linear_allocator_tests.cpp"
	"src/containers/hash_table_tests.h" "src/containers/hash_table_tests.cpp"
)

target_link_libraries(Tests PUBLIC C3DEngine)

add_custom_target(CopyDLLTests
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/tests"
	DEPENDS C3DEngine
)

add_dependencies(Tests CopyDLLTests)