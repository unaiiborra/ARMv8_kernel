#pragma once


#if defined(__GNUC__) || defined(__clang__)
#    define expect(x, expected) __builtin_expect((x), expected)
#    define likely(x)           expect(!!(x), 1)
#    define unlikely(x)         expect(!!(x), 0)
#else
#    define expect(x, expected) (x)
#    define likely(x)           (x)
#    define unlikely(x)         (x)
#endif
