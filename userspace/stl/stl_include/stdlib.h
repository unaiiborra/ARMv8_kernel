#pragma once

#include <stdint.h>

/* ── API ────────────────────────────────────────────────────────────────── */

__attribute__((noreturn)) void exit(int32_t code);
void                           yield(void);