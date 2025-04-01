cmake_minimum_required(VERSION 3.28)

project(CoroFX
    VERSION 0.1.0
    LANGUAGES CXX)

option(COROFX_ENABLE_HARDENING "Enable hardening" ON)

option(COROFX_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(COROFX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(COROFX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddExample)
include(AddTest)

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
        src/trace.cpp)

if(PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(CoroFX PUBLIC
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<BOOL:${COROFX_ENABLE_HARDENING}>:
                _FORTIFY_SOURCE=3
                _GLIBCXX_ASSERTIONS>>)
    target_compile_options(CoroFX PUBLIC
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            -Wall -Wconversion -Werror -Wextra -Wformat -Wformat=2 -Wpedantic -Wsign-conversion
            -fno-rtti -fno-semantic-interposition
            -fstrict-flex-arrays=3
            -fvisibility=hidden
            $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>:
                -mbranch-protection=standard>
            $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:
                -fcf-protection=full>
            $<$<CONFIG:Release>:
                -fno-delete-null-pointer-checks
                -fno-strict-aliasing -fno-strict-overflow
                -ftrivial-auto-var-init=zero>
            $<$<BOOL:${COROFX_ENABLE_HARDENING}>:
                -fstack-clash-protection
                -fstack-protector-strong>
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${COROFX_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>>
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:/fsanitize=address>>)
    target_link_options(CoroFX PUBLIC
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            "LINKER:--as-needed,--no-copy-dt-needed-entries,-z,nodlopen,-z,noexecstack,-z,now,-z,relro"
            $<$<CONFIG:Release>:-s>
            $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${COROFX_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>>)
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

file(GLOB_RECURSE COROFX_ALL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

add_custom_target(format
    COMMAND clang-format -i ${COROFX_ALL_FILES}
    COMMENT "Formatting code...")
