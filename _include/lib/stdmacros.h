#pragma once

#define loop while (1)


#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))


#define TYPE_BIT_SIZE(type) (sizeof(type) * 8)


#define dbg_var(T) __attribute((unused)) T