#pragma once

#include <stdint.h>
#include <stdnoreturn.h>

/* ── API ────────────────────────────────────────────────────────────────── */

noreturn void exit(int32_t code);
void          yield(void);