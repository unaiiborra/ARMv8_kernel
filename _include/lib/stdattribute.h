#pragma once

#define STD_ATTRIBUTE

#define attr(...)     [[__VA_ARGS__]]
#define attrT(T, ...) attr(__VA_ARGS__) T


#define defer(fn)        attr(gnu::__cleanup__(fn))
#define deferT(T, defer) attrT(T, gnu::__cleanup__(defer))



// debug
#define dbgT(T) [[maybe_unused]] T

#ifdef DEBUG
#    define dbg_mode() 1
#    define dbg(code)  code
#else
#    define dbg_mode() 0
#    define dbg(code)
#endif
