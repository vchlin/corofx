cmake_minimum_required(VERSION 3.28)

project(CoroFX
    VERSION 0.1.0
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddExample)
include(AddTest)

option(COROFX_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(COROFX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(COROFX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

include(CheckPIESupported)
check_pie_supported()

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
        include/corofx/promise.hpp
        include/corofx/task.hpp
        include/corofx/trace.hpp
    PRIVATE
        src/check.cpp
        src/detail/type_set.cpp
        src/effect.cpp
        src/promise.cpp
        src/task.cpp
        src/trace.cpp)

if(PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(CoroFX PUBLIC
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            _FORTIFY_SOURCE=3
            _GLIBCXX_ASSERTIONS>)
    target_compile_options(CoroFX INTERFACE
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            -Wall -Wconversion -Werror -Wextra -Wformat=2 -Wpedantic -Wsign-conversion
            -fno-rtti -fno-semantic-interposition
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${COROFX_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>
            -fstack-clash-protection
            -fstack-protector-strong
            -fstrict-flex-arrays=3
            -ftrivial-auto-var-init=zero
            -fvisibility=hidden>
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:/fsanitize=address>>)
    target_link_options(CoroFX INTERFACE
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:Release>:-s>
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${COROFX_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>
            "LINKER:-z,nodlopen,-z,noexecstack,-z,now,-z,relro" >
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:/fsanitize=address>>)
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(examples)
        add_subdirectory(tests)
    endif()

    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfigVersion.cmake"
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(
        TARGETS CoroFX
        EXPORT CoroFXTargets
        FILE_SET HEADERS
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(
        EXPORT CoroFXTargets
        FILE CoroFXTargets.cmake
        NAMESPACE CoroFX::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX")
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CoroFXConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CoroFX")
endif()
