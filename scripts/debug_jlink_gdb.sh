#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname "$SCRIPT_DIR")

DEVICE="MIMX8MN6_A53_0"
SPEED=4000
IFACE=JTAG
ELF="$PROJECT_DIR/bin/kernel.elf"
GDB=gdb

LOW_TEXT=0x40200000
HIGH_TEXT=0xffff800040200000

echo "Connecting to JTAG..."

JLinkGDBServer -if $IFACE -device $DEVICE -speed $SPEED &

if [[ -e /dev/ttyACM0 ]]; then
    ghostty -e bash -c "picocom -q -b 115200 /dev/ttyACM0 && exit" &
fi

if [[ -e /dev/ttyCH343USB0 ]]; then
    ghostty -e bash -c "picocom -q -b 115200 /dev/ttyCH343USB0 && exit" &
fi

mkdir -p "$PROJECT_DIR/tmp"

GDB_SCRIPT="$PROJECT_DIR/tmp/full.gdb"

sed \
  -e "s|__ENTRY__|$LOW_TEXT|g" \
  -e "s|__ELF_PATH__|$ELF|g" \
  "$SCRIPT_DIR/config.gdb" > "$GDB_SCRIPT"
  
ghostty -e bash -c "$GDB $ELF -x $GDB_SCRIPT"

pkill JLinkGDBServer
pkill picocom
rm -rf "$PROJECT_DIR/tmp"