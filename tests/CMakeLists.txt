function(corofx_add_test test_name)
    add_executable(${test_name})
    target_sources(${test_name} PRIVATE ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE CoroFX ${ARGN})
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COROFX_ENABLE_COVERAGE)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "LLVM_PROFILE_FILE=${test_name}.profraw")
    endif()
endfunction()

corofx_add_test(test_chained)
corofx_add_test(test_combined)
corofx_add_test(test_move)
corofx_add_test(test_nested)
# GCC 13.3.0 seems to have some issues with symmetric transfer when sanitizers are enabled.
# Likely https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897.
if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR COROFX_ENABLE_ASAN OR COROFX_ENABLE_TSAN)))
    corofx_add_test(test_recursive)
endif()
corofx_add_test(test_task_move)
corofx_add_test(test_type_set)
