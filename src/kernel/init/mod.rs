pub type KernelInitcall = fn();

const RUST_KERNEL_INITCALLS: &[KernelInitcall] = &[];

#[unsafe(no_mangle)]
extern "C" fn rust_kernel_initcalls() {
    for f in RUST_KERNEL_INITCALLS.iter() {
        f();
    }
}
