target remote localhost:__PORT__
set print pretty on
monitor halt

load

set $load_offset = (long)__ENTRY__ - (long)__HI_TEXT__
add-symbol-file __ELF_PATH__ -o $load_offset

set $pc = __ENTRY__

layout split

b *((long)kernel_entry - (long)0xffff800000000000)

b *((long)mm_reloc - (long)0xffff800000000000)
commands
    silent
    delete 1
    delete 2
    symbol-file
    add-symbol-file __ELF_PATH__
    echo [MM] Kernel relocated\n

    b kernel_entry
    
    
    b smp_gdb_thread_detect
    commands
        silent
        monitor imx8mp.a53.1 arp_examine
        monitor imx8mp.a53.2 arp_examine
        monitor imx8mp.a53.3 arp_examine
    
        set scheduler-locking off

        disconnect
        target remote localhost:__PORT__
               
        continue thread 1
        continue thread 2
        continue thread 3
        continue thread 4

        set scheduler-locking on
        echo SMP finished\n
    end

    continue
end

define thl
    set scheduler-locking on
end

define thu
    set scheduler-locking off
end

define unlock
    if $arg0 != -1
        set smp_gdb_barrier_hang[$arg0 - 1] = 0
    else
        set smp_gdb_barrier_hang[0] = 0
        set smp_gdb_barrier_hang[1] = 0
        set smp_gdb_barrier_hang[2] = 0
        set smp_gdb_barrier_hang[3] = 0
    end
end