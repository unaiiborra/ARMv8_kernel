#!/bin/bash

DEVICE="MIMX8MN6_A53_0"
SPEED=4000
IFACE=JTAG
ELF="/home/unab/files/master/tfm/imx8mp/project/bin/kernel.elf"
GDB=gdb

LOW_TEXT=0x40200000
HIGH_TEXT=0xffff800040200000


echo "Connecting to JTAG..."

JLinkGDBServer -if $IFACE -device $DEVICE -speed $SPEED &

if [[ -e /dev/ttyACM0 ]]; then
    kitty --hold --title "ACM0" picocom -q -b 115200 /dev/ttyACM0 &
fi

if [[ -e /dev/ttyCH343USB0 ]]; then
    kitty --hold --title "CH343USB0" picocom -q -b 115200 /dev/ttyCH343USB0 &
fi

kitty --title "GDB A53" \
    $GDB $ELF \
        -x /home/unab/files/master/tfm/imx8mp/project/scripts/config.gdb

pkill JLinkGDBServer

exit