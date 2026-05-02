#pragma once

#define STD_ATTRIBUTE

#define attr(...)     [[__VA_ARGS__]]
#define attrT(T, ...) attr(__VA_ARGS__) T


#define defer(fn)        attr(gnu::__cleanup__(fn))
#define deferT(T, defer) attrT(T, gnu::__cleanup__(defer))



// debug
#define dbgT(T) attr(maybe_unused) T

#ifdef DEBUG
#    define dbg_mode() 1
#    define dbg(code)  code
#else
#    define dbg_mode() 0
#    define dbg(code)
#endif


// safe for execution in early stages (with mmu disabled and alignment check
// enforced)
#define safe_early attr(gnu::target("general-regs-only"), gnu::cold)