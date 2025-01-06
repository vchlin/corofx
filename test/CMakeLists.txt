add_executable(TestIO)
target_sources(TestIO PRIVATE test_io.cpp)
target_link_libraries(TestIO PRIVATE CoroFX CommonConfig)
add_test(NAME TestIO COMMAND $<TARGET_FILE:TestIO>)
