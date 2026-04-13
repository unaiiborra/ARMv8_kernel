.section .text

.align 4

.global _smc_call
// x0 = uint32_t fid, x1 = uint64_t arg1, x2 = uint64_t arg2, x3 = uint64_t arg3, 
// x4 = uint64_t arg4, x5 = uint64_t arg5, x6 = uint64_t arg6, x7 = uint64_t arg7
// x8 = struct smccc_res_t*
_smc_call:
    smc #0
    
    stp x0, x1, [x8] 
    stp x2, x3, [x8, #16]
    ret


