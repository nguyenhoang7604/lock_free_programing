enable_testing()

set(sources
	test_lock_free_queue_hazard.cpp
)

list(SORT sources)

add_executable(lock_free_container_gtest ${sources})

target_link_libraries(lock_free_container_gtest
	GTest::gmock
	GTest::gmock_main
	GTest::gtest
	GTest::gtest_main
	pthread
)

# Include your project headers (adjust path as needed)
target_include_directories(lock_free_container_gtest PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

# Optionally register the tests
include(GoogleTest)
gtest_discover_tests(lock_free_container_gtest)

add_test(lock_free_container_gtest lock_free_container_gtest)
add_custom_command(TARGET lock_free_container_gtest POST_BUILD
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lock_free_container_gtest -d)

install(TARGETS lock_free_container_gtest
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
