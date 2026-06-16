# made with help of an llm

import math

PAGE_SIZE = 4096
UINT64_SIZE = 8
PTR_SIZE = 8
BITFIELD64_SIZE = 8

CACHE_SIZES = [8, 16, 32, 64, 128, 256, 512, 1024]


def entry_size(cache_malloc_size):
    # Prevent ENTRY_SIZE = 0
    return max(1, UINT64_SIZE // cache_malloc_size)


def analyze_cache(cache_malloc_size, max_pages):
    print(f"\n=== CACHE_MALLOC_{cache_malloc_size} ===")

    for pages in range(1, max_pages + 1):
        total_bytes = pages * PAGE_SIZE

        # Fixed struct overhead
        pointer_bytes = 2 * PTR_SIZE
        usable_bytes = total_bytes - pointer_bytes

        bytes_per_entry = cache_malloc_size

        # Maximum entries ignoring bitfields
        max_entries = usable_bytes // bytes_per_entry

        # Bitfields (1 bit per entry)
        bitfields_needed = math.ceil(max_entries / 64)
        bitfield_bytes = bitfields_needed * BITFIELD64_SIZE

        # Recompute after accounting for bitfields
        usable_after_bitfield = usable_bytes - bitfield_bytes
        real_entries = usable_after_bitfield // bytes_per_entry

        buffer_bytes = real_entries * bytes_per_entry

        # Total struct usage (everything except unused slack)
        struct_used_bytes = buffer_bytes + bitfield_bytes + pointer_bytes

        buffer_utilization = (buffer_bytes / total_bytes) * 100
        struct_utilization = (struct_used_bytes / total_bytes) * 100

        print(
            f"Pages: {pages:2d} | "
            f"Entries: {real_entries:6d} | "
            f"Bitfields: {bitfields_needed:5d} | "
            f"Struct bytes: {total_bytes:6d} | "
            f"Buffer bytes: {buffer_bytes:6d} | "
            f"Buffer util: {buffer_utilization:6.2f}% | "
            f"Struct util: {struct_utilization:6.2f}%"
        )


def main():
    max_pages = int(input("Maximum number of pages N: "))

    for cache_size in CACHE_SIZES:
        analyze_cache(cache_size, max_pages)


if __name__ == "__main__":
    main()
