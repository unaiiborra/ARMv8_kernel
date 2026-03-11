#!/usr/bin/env python3
import sys
import os

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file.elf>")
        sys.exit(1)

    elf_path = sys.argv[1]

    if not os.path.isfile(elf_path):
        print(f"Error: file not found: {elf_path}")
        sys.exit(1)

    with open(elf_path, "rb") as f:
        data = f.read()

    out_path = elf_path + ".txt"

    with open(out_path, "w") as out:
        out.write("/* Auto-generated from %s */\n" % os.path.basename(elf_path))
        out.write("/* Size: %d bytes */\n\n" % len(data))

        out.write("const unsigned long test_elf_size = %d;\n" % len(data))

        out.write("const unsigned char test_elf[] = {\n")
        for i, b in enumerate(data):
            out.write(f"0x{b:02x},")
            if (i + 1) % 16 == 0:
                out.write("\n")
            else:
                out.write(" ")
        out.write("\n};\n\n")
        


    print(f"Generated: {out_path}")
    print(f"Size: {len(data)} bytes")


if __name__ == "__main__":
    main()