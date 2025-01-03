#pragma once

#ifdef _WIN32
#define COROFX_PUBLIC __declspec(dllexport)
#elif __GNUC__ >= 4
#define COROFX_PUBLIC __attribute__((visibility("default")))
#else
#define COROFX_PUBLIC
#endif

#ifdef __GNUC__
#define COROFX_INLINE __attribute__((always_inline))
#else
#define COROFX_INLINE
#endif
