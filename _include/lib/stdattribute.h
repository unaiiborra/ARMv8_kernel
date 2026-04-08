#pragma once

#define STD_ATTRIBUTE

#define attr(...)     __attribute__((__VA_ARGS__))
#define attrT(T, ...) attr(__VA_ARGS__) T


#define defer(fn)        attr(cleanup(fn))
#define deferT(T, defer) attrT(T, cleanup(defer))



// debug
#define dbgT(T) attrT(T, unused)

#ifdef DEBUG
#    define dbg_mode() 1
#    define dbg(code)  code
#else
#    define dbg_mode() 0
#    define dbg(code)
#endif
