corofx_add_test(test_chained CoroFX PrivateConfig)
corofx_add_test(test_combined CoroFX PrivateConfig)
corofx_add_test(test_nested CoroFX PrivateConfig)
# GCC 13.3.0 seems to have some issues with symmetric transfer when sanitizers are enabled.
if (NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND (${COROFX_ENABLE_ASAN} OR ${COROFX_ENABLE_TSAN})))
    corofx_add_test(test_recursive CoroFX PrivateConfig)
endif()
corofx_add_test(test_type_set CoroFX PrivateConfig)
