.section .text

# #[repr(C)]
# struct ThreadArgs {
#     arg: u64,
#     entry: *const c_void,
# }

.global __stl_thread_start
__stl_thread_start:
    mov x10, x0 // thid
    mov x11, x1 // arguments
    bl __stl_setup_stack

    mov x29, #0
    mov x30, #0

    mov x0, x10
    mov x1, x11
    b __stl_setup_thread_entry