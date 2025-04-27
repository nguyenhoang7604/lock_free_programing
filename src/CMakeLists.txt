set(EXECUTABLES
	spsc_lockfree_queue
	mpmc_lockfree_queue
)

foreach(exec IN LISTS EXECUTABLES)
	add_executable(${exec} ${exec}.cpp)
	target_link_libraries(${exec} PUBLIC pthread)
	target_include_directories(${exec} PRIVATE ${PROJECT_INCLUDE_DIR})
	install(TARGETS ${exec} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()

add_executable(boost_queue_mpmc boost_queue_mpmc.cpp)

target_link_libraries(boost_queue_mpmc PRIVATE Boost::boost)

target_include_directories(boost_queue_mpmc PRIVATE ${Boost_INCLUDE_DIRS})

install(TARGETS boost_queue_mpmc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
