#pragma once

#include <lib/stdint_extra.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/// Copies a string into another buffer until the max_size or until finding a
/// \0. It allways appends a \0 either at max_size -1 or where it finds it in
/// the source
void strcopy(char* dst, const char* src, uint64_t max_size);


/// Compares two strings and tells if the contents are equal
bool strcmp(const char* a, const char* b);


/// Converts a uint8_t to a char, as it only converts to one char, it panics
/// if value is > 9
char uint8_t_to_ascii_char(uint8_t n);

#define INT8_TO_ASCII_BUF 5  // "-128" + '\0'
#define UINT8_TO_ASCII_BUF 4 // "255"  + '\0'

#define INT16_TO_ASCII_BUF 7  // "-32768" + '\0'
#define UINT16_TO_ASCII_BUF 6 // "65535"  + '\0'

#define INT32_TO_ASCII_BUF 12  // "-2147483648" + '\0'
#define UINT32_TO_ASCII_BUF 11 // "4294967295"  + '\0'

#define INT64_TO_ASCII_BUF 21  // "-9223372036854775808" + '\0'
#define UINT64_TO_ASCII_BUF 21 // "18446744073709551615" + '\0'

/// Converts any stdint, signed or not to its ascii decimal representation.
/// Panics if provided buf_len is smaller than requiered. Space for the '-' sign
/// and '\0' is needed
char* stdint_to_ascii(
    STDINT_UNION n,
    STDINT_TYPES n_type,
    char* buf,
    uint64_t buf_len,
    STDINT_BASE_REPR repr);


void test_stdint_to_ascii(int64_t test_v, uint64_t buf_size);


typedef void (*str_fmt_putc)(char c, void* args);
void str_fmt_print(str_fmt_putc putc, void* args, const char* s, va_list ap);
