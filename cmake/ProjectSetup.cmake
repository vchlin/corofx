function(project_add_common_options prefix)
    string(TOUPPER ${prefix} prefix_upper)

    option(${prefix_upper}_ENABLE_COVERAGE "Enable coverage" OFF)
    option(${prefix_upper}_ENABLE_HARDENING "Enable hardening" ON)

    option(${prefix_upper}_ENABLE_ASAN "Enable AddressSanitizer" OFF)
    option(${prefix_upper}_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
    option(${prefix_upper}_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

    add_library(${prefix}BuildOptions INTERFACE)
    target_compile_definitions(${prefix}BuildOptions INTERFACE
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<BOOL:${${prefix_upper}_ENABLE_HARDENING}>:
                _FORTIFY_SOURCE=3
                _GLIBCXX_ASSERTIONS
            >
        >
    )
    target_compile_options(${prefix}BuildOptions INTERFACE
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            -Wall -Wconversion -Werror -Wextra -Wformat -Wformat=2 -Wpedantic -Wsign-conversion
            -fno-rtti -fno-semantic-interposition
            -fstrict-flex-arrays=3
            -fvisibility=hidden
            $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>:-mbranch-protection=standard>
            $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:-fcf-protection=full>
            $<$<CONFIG:Release>:
                -fno-delete-null-pointer-checks
                -fno-strict-aliasing -fno-strict-overflow
                -ftrivial-auto-var-init=zero
            >
            $<$<BOOL:${${prefix_upper}_ENABLE_HARDENING}>:
                -fstack-clash-protection
                -fstack-protector-strong
            >
            $<$<BOOL:${${prefix_upper}_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${${prefix_upper}_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${${prefix_upper}_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>>
        $<$<CXX_COMPILER_ID:Clang>:
            $<$<BOOL:${${prefix_upper}_ENABLE_COVERAGE}>:-fcoverage-mapping -fprofile-instr-generate>
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<BOOL:${${prefix_upper}_ENABLE_ASAN}>:/fsanitize=address>
        >
    )
    target_link_options(${prefix}BuildOptions INTERFACE
        $<$<CXX_COMPILER_ID:Clang,GNU>:
            "LINKER:--as-needed,--no-copy-dt-needed-entries,-z,nodlopen,-z,noexecstack,-z,now,-z,relro"
            $<$<CONFIG:Release>:-s>
            $<$<BOOL:${${prefix_upper}_ENABLE_ASAN}>:-fsanitize=address>
            $<$<BOOL:${${prefix_upper}_ENABLE_TSAN}>:-fsanitize=thread>
            $<$<BOOL:${${prefix_upper}_ENABLE_UBSAN}>:-fsanitize=undefined -fno-sanitize-recover>
        >
        $<$<CXX_COMPILER_ID:Clang>:
            $<$<BOOL:${${prefix_upper}_ENABLE_COVERAGE}>:-fcoverage-mapping -fprofile-instr-generate>
        >
    )
endfunction()
