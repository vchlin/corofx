cmake_minimum_required(VERSION 3.28)

project(CoroFX
    VERSION 0.1.0
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckPIESupported)
check_pie_supported()

include(MyBuildOptions)
my_add_build_options(CoroFX)

add_library(CoroFX)
add_library(CoroFX::CoroFX ALIAS CoroFX)
set_target_properties(CoroFX PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_features(CoroFX PUBLIC cxx_std_20)
target_sources(CoroFX
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/corofx/check.hpp
        include/corofx/config.hpp
        include/corofx/detail/type_set.hpp
        include/corofx/effect.hpp
        include/corofx/frame.hpp
        include/corofx/handler.hpp
        include/corofx/promise.hpp
        include/corofx/task.hpp
        include/corofx/trace.hpp
    PRIVATE
        src/check.cpp
        src/detail/type_set.cpp
        src/effect.cpp
        src/frame.cpp
        src/handler.cpp
        src/promise.cpp
        src/task.cpp
        src/trace.cpp
)

if(PROJECT_IS_TOP_LEVEL)
    target_link_libraries(CoroFX PUBLIC CoroFXBuildOptions)

    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(examples)
        add_subdirectory(tests)
    endif()

    # Installation.
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX"
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfigVersion.cmake"
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(
        TARGETS CoroFX CoroFXBuildOptions
        EXPORT CoroFXTargets
        FILE_SET HEADERS
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    install(
        EXPORT CoroFXTargets
        FILE CoroFXTargets.cmake
        NAMESPACE CoroFX::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX"
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX"
    )

    # Formatting.
    file(GLOB_RECURSE COROFX_ALL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )
    add_custom_target(format
        COMMAND clang-format -i ${COROFX_ALL_FILES}
        COMMENT "Formatting code..."
    )

    # Coverage.
    get_property(COROFX_ALL_TESTS DIRECTORY tests PROPERTY TESTS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COROFX_ENABLE_COVERAGE)
        include(MyLLVMTools)
        my_get_llvm_version(MAJOR LLVM_VERSION_MAJOR)
        add_custom_target(coverage
            COMMAND llvm-profdata-${LLVM_VERSION_MAJOR} merge -sparse *.profraw -o tests.profdata
            COMMAND llvm-cov-${LLVM_VERSION_MAJOR} report ${COROFX_ALL_TESTS} -instr-profile=tests.profdata
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests"
        )
    endif()
endif()
