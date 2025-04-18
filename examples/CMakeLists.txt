function(corofx_add_example example_name)
    add_executable(${example_name})
    target_sources(${example_name} PRIVATE ${example_name}.cpp)
    target_link_libraries(${example_name} PRIVATE CoroFX ${ARGN})
endfunction()

corofx_add_example(raise)
corofx_add_example(yield)
