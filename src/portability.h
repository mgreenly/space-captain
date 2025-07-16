#ifndef PORTABILITY_H
#define PORTABILITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Struct Packing Macros
// ============================================================================
// These macros ensure structures are packed without padding across different
// compilers. Usage:
//   PACKED_STRUCT_BEGIN
//   typedef struct PACKED_ATTR {
//     uint16_t field1;
//     uint32_t field2;
//   } my_packed_struct_t;
//   PACKED_STRUCT_END

#ifdef _MSC_VER
// Microsoft Visual C++
#define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
#define PACKED_STRUCT_END   __pragma(pack(pop))
#define PACKED_ATTR
#else
// GCC, Clang, and other compilers supporting __attribute__
#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END
#define PACKED_ATTR __attribute__((packed))
#endif

// ============================================================================
// Safe Arithmetic Operations
// ============================================================================
// These macros provide overflow-safe arithmetic operations across different
// compilers.

#if defined(__GNUC__) || defined(__clang__)
// GCC and Clang have built-in overflow checking
#define SC_MUL_OVERFLOW(a, b, result) __builtin_mul_overflow(a, b, result)
#define SC_ADD_OVERFLOW(a, b, result) __builtin_add_overflow(a, b, result)
#define SC_SUB_OVERFLOW(a, b, result) __builtin_sub_overflow(a, b, result)
#else
// Portable overflow checking for multiplication
static inline bool sc_mul_overflow_size_t(size_t a, size_t b, size_t *result) {
  if (a != 0 && b > SIZE_MAX / a) {
    return true; // overflow would occur
  }
  *result = a * b;
  return false; // no overflow
}

// Portable overflow checking for addition
static inline bool sc_add_overflow_size_t(size_t a, size_t b, size_t *result) {
  if (b > SIZE_MAX - a) {
    return true; // overflow would occur
  }
  *result = a + b;
  return false; // no overflow
}

// Portable overflow checking for subtraction
static inline bool sc_sub_overflow_size_t(size_t a, size_t b, size_t *result) {
  if (b > a) {
    return true; // underflow would occur
  }
  *result = a - b;
  return false; // no underflow
}

// Map generic macros to type-specific functions
// Note: This is simplified for size_t only. A full implementation would
// need _Generic (C11) or separate macros for each type.
#define SC_MUL_OVERFLOW(a, b, result) sc_mul_overflow_size_t(a, b, result)
#define SC_ADD_OVERFLOW(a, b, result) sc_add_overflow_size_t(a, b, result)
#define SC_SUB_OVERFLOW(a, b, result) sc_sub_overflow_size_t(a, b, result)
#endif

// ============================================================================
// Thread Local Storage
// ============================================================================
// Already handled by using C11's thread_local directly

// ============================================================================
// Other Compiler-Specific Features
// ============================================================================

// Inline functions
#ifdef _MSC_VER
#define SC_INLINE __inline
#else
#define SC_INLINE inline
#endif

// Unused parameter marking
#ifdef __GNUC__
#define SC_UNUSED __attribute__((unused))
#else
#define SC_UNUSED
#endif

// Function attributes for optimization hints
#ifdef __GNUC__
#define SC_PURE_FUNC  __attribute__((pure))
#define SC_CONST_FUNC __attribute__((const))
#else
#define SC_PURE_FUNC
#define SC_CONST_FUNC
#endif

#endif // PORTABILITY_H
