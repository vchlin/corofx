cmake_minimum_required(VERSION 3.28)

project(CoroFX
    VERSION 0.1
    LANGUAGES CXX)

include(CheckPIESupported)
check_pie_supported()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddExample)
include(AddTest)

option(COROFX_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(COROFX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

add_library(CommonConfig INTERFACE)
target_compile_definitions(CommonConfig INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:
        $<$<CONFIG:Release>:NDEBUG>
        _FORTIFY_SOURCE=3
        _GLIBCXX_ASSERTIONS>)
target_compile_features(CommonConfig INTERFACE cxx_std_20)
target_compile_options(CommonConfig INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:
        $<$<CONFIG:Release>:-O2>
        -Wall -Wconversion -Werror -Wextra -Wformat=2 -Wsign-conversion
        -fno-rtti -fno-semantic-interposition
        $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address,undefined>
        $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread,undefined>
        -fstack-clash-protection
        #-fstrict-flex-arrays=3
        -fstack-protector-strong
        -ftrivial-auto-var-init=zero
        -fvisibility=hidden>)
target_link_options(CommonConfig INTERFACE
    $<$<CXX_COMPILER_ID:Clang,GNU>:
        $<$<CONFIG:Release>:-s>
        $<$<BOOL:${COROFX_ENABLE_ASAN}>:-fsanitize=address,undefined>
        $<$<BOOL:${COROFX_ENABLE_TSAN}>:-fsanitize=thread,undefined>
        "LINKER:-z,nodlopen,-z,noexecstack,-z,now,-z,relro" >)

add_library(CoroFX)
set_target_properties(CoroFX PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(CoroFX
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/corofx/check.hpp
        include/corofx/config.hpp
        include/corofx/detail/type_id.hpp
        include/corofx/detail/type_set.hpp
        include/corofx/promise.hpp
        include/corofx/task.hpp
        include/corofx/trace.hpp
    PRIVATE
        src/check.cpp
        src/detail/type_id.cpp
        src/detail/type_set.cpp
        src/task.cpp
        src/trace.cpp)
target_link_libraries(CoroFX PRIVATE CommonConfig)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(example)
    add_subdirectory(test)
endif()
