#ifndef BANJO_BINDGEN_STDINT_H
#define BANJO_BINDGEN_STDINT_H

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#if __wasm32__
typedef uint32_t size_t;
typedef int32_t ssize_t;
#else
typedef uint64_t size_t;
typedef int64_t ssize_t;
#endif

#endif
