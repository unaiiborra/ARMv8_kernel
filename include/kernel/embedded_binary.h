#pragma once

#define _EMBED_SYM(name, suffix) _embed_##name##_##suffix

#define EMBEDDED_BINARY(name)                              \
    ({                                                     \
        void* binary = (void*)EMBEDDED_BINARY_START(name); \
        binary;                                            \
    })

#define EMBEDDED_BINARY_START(name)                     \
    ({                                                  \
        extern const uint8_t _EMBED_SYM(name, start)[]; \
        _EMBED_SYM(name, start);                        \
    })

#define EMBEDDED_BINARY_END(name)                     \
    ({                                                \
        extern const uint8_t _EMBED_SYM(name, end)[]; \
        _EMBED_SYM(name, end);                        \
    })

#define EMBEDDED_BINARY_SIZE(name)                                 \
    ({                                                             \
        extern const uint8_t _EMBED_SYM(name, start)[];            \
        extern const uint8_t _EMBED_SYM(name, end)[];              \
        (size_t)(_EMBED_SYM(name, end) - _EMBED_SYM(name, start)); \
    })
