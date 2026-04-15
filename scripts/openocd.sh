#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname "$SCRIPT_DIR")

ELF="$PROJECT_DIR/bin/kernel.elf"
GDB=gdb
PORT=3333
LOW_TEXT=0x40200000
HI_TEXT=0xFFFF800040200000
HI_BASE=0xFFFF800000000000


cleanup() {
    echo "Cerrando..."
    [[ -n $OPENOCD_PID ]] && kill $OPENOCD_PID 2>/dev/null
    [[ -n $SERIAL1_PID ]] && kill $SERIAL1_PID 2>/dev/null
    [[ -n $SERIAL2_PID ]] && kill $SERIAL2_PID 2>/dev/null
    rm -rf "$PROJECT_DIR/tmp"
    exit 0
}
trap cleanup EXIT INT TERM

openocd -f scripts/openocd.cfg 2>&1 &
OPENOCD_PID=$!

sleep 1

mkdir -p "$PROJECT_DIR/tmp"
touch $PROJECT_DIR/tmp/full.gdb
GDB_SCRIPT="$PROJECT_DIR/tmp/full.gdb"

if [[ -e /dev/ttyACM0 ]]; then
    ghostty -e bash -c "picocom -q -b 115200 /dev/ttyACM0 && exit" &
    SERIAL1_PID=$!
fi

if [[ -e /dev/ttyCH343USB0 ]]; then
    ghostty -e bash -c "picocom -q -b 115200 /dev/ttyCH343USB0 && exit" &
    SERIAL2_PID=$!
fi

sed \
  -e "s|__ENTRY__|$LOW_TEXT|g" \
  -e "s|__ELF_PATH__|$ELF|g" \
  -e "s|__PORT__|$PORT|g" \
  -e "s|__LO_TEXT__|$LOW_TEXT|g" \
  -e "s|__HI_TEXT__|$HI_TEXT|g" \
  "$SCRIPT_DIR/config.gdb" > "$GDB_SCRIPT"

# GDB en foreground — cuando el usuario cierra la ventana, cleanup se dispara
ghostty -e bash -c "$GDB $ELF -x $GDB_SCRIPT"