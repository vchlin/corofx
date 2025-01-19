add_executable(generator)
target_sources(generator PRIVATE generator.cpp)
target_link_libraries(generator PRIVATE CoroFX CommonConfig)
