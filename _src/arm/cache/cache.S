.section .text

// x0 = void* start, x1 = void* end
.global _cache_flush_range
_cache_flush_range: 
    tst x0, #63
    b.ne hang
    tst x1, #63
    b.ne hang

1:
    cmp x1, x0
    b.le 2f

    ic ivau, x0
    dc cvau, x0

    add x0, x0, #64
    b 1b

2:
    ic iallu
    dsb ish
    isb
    
    ret

hang:
    wfe
    b hang
    