file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(run_test ${test_sources})
find_package(Catch2)
target_link_libraries(run_test Catch2::Catch2 ${PROJECT_NAME}_lib)