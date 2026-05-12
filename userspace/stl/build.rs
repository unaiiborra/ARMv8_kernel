use dotenvy::dotenv;
use glob::glob;
use std::path::Path;

struct DotEnv {
    pub cross_compile_path: String,
}
impl DotEnv {
    pub fn read() -> Self {
        dotenv().ok();

        let cross_compile_path =
            std::env::var("CROSS_COMPILE_PATH").expect("CROSS_COMPILE_PATH not defined");

        Self { cross_compile_path }
    }
}

fn main() {
    println!("cargo:note=Building STL...");

    let env = DotEnv::read();

    let asm_files: Vec<_> = glob("src/**/*.S").unwrap().filter_map(|e| e.ok()).collect();
    let c_files: Vec<_> = glob("src/**/*.c").unwrap().filter_map(|e| e.ok()).collect();
    let cpp_files: Vec<_> = glob("src/**/*.cpp")
        .unwrap()
        .filter_map(|e| e.ok())
        .collect();

    let opt = if cfg!(debug_assertions) { 0 } else { 2 };
    let export_debug_symbols = cfg!(debug_assertions);

    let includes = [
        Path::new(&env.cross_compile_path).join("include"),
        Path::new("stl_include").to_path_buf(),
    ];

    let mut cxflags: Vec<&str> = Vec::from([
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ffreestanding",
        "-nostdlib",
        "-nostartfiles",
        "-nostdinc",
    ]);
    if export_debug_symbols {
        cxflags.append(&mut Vec::from(["-g3", "-DDEBUG=1", "-DTESTING"]));
    }

    let cflags = ["-std=gnu23"];
    let cppflags = ["-std=gnu++20", "-fno-exceptions", "-fno-rtti"];

    if !asm_files.is_empty() {
        cc::Build::new()
            .compiler("aarch64-none-elf-gcc")
            .flag("-x")
            .flag("assembler-with-cpp")
            .files(asm_files)
            .compile("asm");
    }

    if !c_files.is_empty() {
        cc::Build::new()
            .compiler("aarch64-none-elf-gcc")
            .includes(includes.clone())
            .flags(cxflags.clone())
            .flags(cflags)
            .opt_level(opt)
            .files(&cpp_files)
            .compile("cpp_sources");
    }

    if !cpp_files.is_empty() {
        cc::Build::new()
            .compiler("aarch64-none-elf-g++")
            .cpp(true)
            .includes(includes.clone())
            .flags(cxflags.clone())
            .flags(cppflags)
            .opt_level(opt)
            .files(&cpp_files)
            .compile("cpp_sources");
    }
}
