#include <drivers/uart/uart.h>
#include <drivers/uart/uart_raw.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/lock/irqlock.h>
#include <stddef.h>
#include <stdint.h>

#include "drivers/uart/raw/uart_usr2.h"
#include "kernel/devices/device.h"
#include "kernel/io/term.h"
#include "lib/mem.h"
#include "lib/stdbitfield.h"
#include "lib/stdmacros.h"


// Rust fns (driver buffer control)

extern bool uart_txbuf_push(const driver_handle* h, uint8_t v);
extern bool uart_rxbuf_push(const driver_handle* h, uint8_t v);
extern bool uart_txbuf_pop(const driver_handle* h, uint8_t* v);
extern bool uart_rxbuf_pop(const driver_handle* h, uint8_t* v);

// Saves which irqs are enabled or disabled, each bitfield represents each
// UART_ID
const uint8_t USR1_IRQ_W1C_BITS_[9] = {
    3,
    4,
    5,
    7,
    8,
    10,
    11,
    12,
    15,
};
const uint8_t USR2_IRQ_W2C_BITS_[8] = {
    1,
    2,
    4,
    7,
    8,
    11,
    12,
    15,
};

static inline uart_state* uart_get_state_(const driver_handle* h)
{
    return (uart_state*)h->state;
}

static inline void uart_check_handle_(const driver_handle* h)
{
    if (!h || !h->state)
        PANIC("uart: invalid handle");
}

// Tx fifo full
static inline bool UART_tx_fifo_full_(uintptr_t base)
{
    UartUtsValue uts = UART_UTS_read(base);

    return UART_UTS_TXFULL_get(uts);
}


void uart_reset(const driver_handle* h)
{
#ifdef TEST
    uart_check_handle_(h);
#endif

    // flush tx fifo
    loop
    {
        UartUsr2Value r = UART_USR2_read(h->base);

        if (UART_USR2_TXFE_get(r) == 1) { // tx fifo empty
            for (size_t i = 0; i < 50000; i++)
                asm volatile("nop");

            break;
        }

        for (size_t i = 0; i < 5000; i++)
            asm volatile("nop");
    }


    UART_UCR2_write(h->base, (UartUcr2Value) {.val = 0});

    while (!(UART_UCR2_read(h->base).val & (1 << 0)))
        asm volatile("nop");

    uart_state* state = uart_get_state_(h);

    state->irq_status = 0;

    state->tx.overwrite = false;
    state->tx.head = 0;
    state->tx.tail = 0;

    state->rx.overwrite = false;
    state->rx.head = 0;
    state->rx.tail = 0;

    for (size_t i = 0; i < UART_TX_BUF_SIZE; i++)
        state->tx.buf[i] = 0;

    for (size_t i = 0; i < UART_RX_BUF_SIZE; i++)
        state->rx.buf[i] = 0;
}

bool uart_read(const driver_handle* h, uint8_t* data)
{
    return uart_rxbuf_pop(h, data);
}

/*
 *  ------------
 *  IRQ HANDLING
 *  ------------
 */

typedef enum {
    UART_IRQ_SRC_START = 0,

    /* =========================================================
     * RX / Receiver related
     * ========================================================= */

    /* RRDYEN (UCR1[9])  -> RRDY (USR1[9]) */
    UART_IRQ_SRC_RRDY = 0, // Rx FIFO ready / threshold reached

    /* IDEN (UCR1[12])  -> IDLE (USR2[12]) */
    UART_IRQ_SRC_IDLE, // Idle line detected

    /* DREN (UCR4[0])   -> RDR (USR2[0]) */
    UART_IRQ_SRC_RDR, // DMA receive request

    /* RXDSEN (UCR3[6]) -> RXDS (USR1[6]) */
    UART_IRQ_SRC_RXDS, // Rx data size

    /* ATEN (UCR2[3])   -> AGTIM (USR1[8]) */
    UART_IRQ_SRC_AGTIM, // Aging timer expired

    /* =========================================================
     * TX / Transmitter related
     * ========================================================= */

    /* TXMPTYEN (UCR1[6]) -> TXFE (USR2[14]) */
    UART_IRQ_SRC_TXFE, // Tx FIFO empty

    /* TRDYEN (UCR1[13]) -> TRDY (USR1[13]) */
    UART_IRQ_SRC_TRDY, // Tx FIFO ready

    /* TCEN (UCR4[3])   -> TXDC (USR2[3]) */
    UART_IRQ_SRC_TXDC, // Transmit complete

    /* =========================================================
     * Error / Modem / Control
     * ========================================================= */

    /* OREN (UCR4[1])   -> ORE (USR2[1]) */
    UART_IRQ_SRC_ORE, // Overrun error

    /* BKEN (UCR4[2])   -> BRCD (USR2[2]) */
    UART_IRQ_SRC_BRCD, // Break detected

    /* WKEN (UCR4[7])   -> WAKE (USR2[7]) */
    UART_IRQ_SRC_WAKE, // Wake-up event

    /* ADEN (UCR1[15])  -> ADET (USR2[15]) */
    UART_IRQ_SRC_ADET, // Auto-baud detect

    /* ACIEN (UCR3[0])  -> ACST (USR2[11]) */
    UART_IRQ_SRC_ACST, // Auto-baud complete

    /* ESCI (UCR2[15])  -> ESCF (USR1[11]) */
    UART_IRQ_SRC_ESC, // Escape sequence

    /* ENIRI (UCR4[8])  -> IRINT (USR2[8]) */
    UART_IRQ_SRC_IRINT, // IR interrupt

    /* AIRINTEN (UCR3[5]) -> AIRINT (USR1[5]) */
    UART_IRQ_SRC_AIRINT, // Auto IR interrupt

    /* AWAKEN (UCR3[4]) -> AWAKE (USR1[4]) */
    UART_IRQ_SRC_AWAKE, // Auto wake

    /* FRAERREN (UCR3[11]) -> FRAERR (USR1[10]) */
    UART_IRQ_SRC_FRAERR, // Framing error

    /* PARERREN (UCR3[12]) -> PARITYERR (USR1[15]) */
    UART_IRQ_SRC_PARITYERR, // Parity error

    /* RTSDEN (UCR1[5]) -> RTSD (USR1[12]) */
    UART_IRQ_SRC_RTSD, // RTS delta

    /* RTSEN (UCR2[4])  -> RTSF (USR2[4]) */
    UART_IRQ_SRC_RTSF, // RTS edge

    /* DTREN (UCR3[13]) -> DTRF (USR2[13]) */
    UART_IRQ_SRC_DTRF, // DTR edge

    /* RI (DTE) (UCR3[8]) -> RIDELT (USR2[10]) */
    UART_IRQ_SRC_RIDELT, // Ring indicator delta

    /* DCD (DTE) (UCR3[9]) -> DCDDELT (USR2[6]) */
    UART_IRQ_SRC_DCDDELT, // DCD delta

    /* DTRDEN (UCR3[3]) -> DTRD (USR1[7]) */
    UART_IRQ_SRC_DTRD, // DTR delta detect

    /* SADEN (UMCR[3])  -> SAD (USR1[3]) */
    UART_IRQ_SRC_SAD, // Stop-after-detect

    UART_IRQ_SRC_COUNT
} UART_IRQ_SOURCE;

/// Enables/Disables the irq
static void
uart_set_irq_state_(const driver_handle* h, UART_IRQ_SOURCE irq, bool enable)
{
#ifdef TEST
    uart_check_handle_(h);
    if (irq >= UART_IRQ_SRC_COUNT)
        PANIC("invalid input");
#endif

    uart_state* state = uart_get_state_(h);


    bitfield_set(state->irq_status, irq, enable);

#define SET_IRQ_CASE(irq, reg, bf, regv_name)     \
    case irq: {                                   \
        regv_name r = UART_##reg##_read(h->base); \
        UART_##reg##_##bf##_set(&r, enable);      \
        UART_##reg##_write(h->base, r);           \
    } break;

    switch (irq) {
        /* ================= RX ================= */
        SET_IRQ_CASE(UART_IRQ_SRC_RRDY, UCR1, RRDYEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_IDLE, UCR1, IDEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_RDR, UCR4, DREN, UartUcr4Value);
        SET_IRQ_CASE(UART_IRQ_SRC_RXDS, UCR3, RXDSEN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_AGTIM, UCR2, ATEN, UartUcr2Value);

        /* ================= TX ================= */
        SET_IRQ_CASE(UART_IRQ_SRC_TXFE, UCR1, TXMPTYEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_TRDY, UCR1, TRDYEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_TXDC, UCR4, TCEN, UartUcr4Value);

        /* ========== Error / Modem / Control ========== */
        SET_IRQ_CASE(UART_IRQ_SRC_ORE, UCR4, OREN, UartUcr4Value);
        SET_IRQ_CASE(UART_IRQ_SRC_BRCD, UCR4, BKEN, UartUcr4Value);
        SET_IRQ_CASE(UART_IRQ_SRC_WAKE, UCR4, WKEN, UartUcr4Value);
        SET_IRQ_CASE(UART_IRQ_SRC_ADET, UCR1, ADEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_ACST, UCR3, ACIEN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_ESC, UCR2, ESCI, UartUcr2Value);
        SET_IRQ_CASE(UART_IRQ_SRC_IRINT, UCR4, ENIRI, UartUcr4Value);
        SET_IRQ_CASE(UART_IRQ_SRC_AIRINT, UCR3, AIRINTEN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_AWAKE, UCR3, AWAKEN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_FRAERR, UCR3, FRAERREN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_PARITYERR, UCR3, PARERREN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_RTSD, UCR1, RTSDEN, UartUcr1Value);
        SET_IRQ_CASE(UART_IRQ_SRC_RTSF, UCR2, RTSEN, UartUcr2Value);
        SET_IRQ_CASE(UART_IRQ_SRC_DTRF, UCR3, DTREN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_RIDELT, UCR3, RI, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_DCDDELT, UCR3, DCD, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_DTRD, UCR3, DTRDEN, UartUcr3Value);
        SET_IRQ_CASE(UART_IRQ_SRC_SAD, UMCR, SADEN, UartUmcrValue);

        default:
            PANIC("unhandled UART irq source");
    }
#undef SET_IRQ_CASE
}

/// Returns the cached irq state value
static inline bool
uart_get_irq_state_(const driver_handle* h, UART_IRQ_SOURCE irq)
{
#ifdef TEST
    uart_check_handle_(h);
    if (irq >= UART_IRQ_SRC_COUNT)
        PANIC("invalid input");
#endif
    bitfield32 irq_status = uart_get_state_(h)->irq_status;

    return (bool)bitfield_get(irq_status, irq);
}

static bitfield32 uart_get_irq_sources(const driver_handle* h)
{
#ifdef TEST
    uart_check_handle_(h);
#endif

    UartUsr1Value usr1 = UART_USR1_read(h->base);
    UartUsr2Value usr2 = UART_USR2_read(h->base);

    bitfield32 sources = 0;

#define SET_SRC(bit, status) \
    sources |= ((bitfield32)(((status) & uart_get_irq_state_(h, bit)) << (bit)))

    SET_SRC(UART_IRQ_SRC_RRDY, UART_USR1_RRDY_get(usr1));
    SET_SRC(UART_IRQ_SRC_IDLE, UART_USR2_IDLE_get(usr2));
    SET_SRC(UART_IRQ_SRC_RDR, UART_USR2_RDR_get(usr2));
    SET_SRC(UART_IRQ_SRC_RXDS, UART_USR1_RXDS_get(usr1));
    SET_SRC(UART_IRQ_SRC_AGTIM, UART_USR1_AGTIM_get(usr1));

    SET_SRC(UART_IRQ_SRC_TXFE, UART_USR2_TXFE_get(usr2));
    SET_SRC(UART_IRQ_SRC_TRDY, UART_USR1_TRDY_get(usr1));
    SET_SRC(UART_IRQ_SRC_TXDC, UART_USR2_TXDC_get(usr2));

    SET_SRC(UART_IRQ_SRC_ORE, UART_USR2_ORE_get(usr2));
    SET_SRC(UART_IRQ_SRC_BRCD, UART_USR2_BRCD_get(usr2));
    SET_SRC(UART_IRQ_SRC_WAKE, UART_USR2_WAKE_get(usr2));
    SET_SRC(UART_IRQ_SRC_ADET, UART_USR2_ADET_get(usr2));
    SET_SRC(UART_IRQ_SRC_ACST, UART_USR2_ACST_get(usr2));
    SET_SRC(UART_IRQ_SRC_ESC, UART_USR1_ESCF_get(usr1));
    SET_SRC(UART_IRQ_SRC_IRINT, UART_USR2_IRINT_get(usr2));
    SET_SRC(UART_IRQ_SRC_AIRINT, UART_USR1_AIRINT_get(usr1));
    SET_SRC(UART_IRQ_SRC_AWAKE, UART_USR1_AWAKE_get(usr1));
    SET_SRC(UART_IRQ_SRC_FRAERR, UART_USR1_FRAERR_get(usr1));
    SET_SRC(UART_IRQ_SRC_PARITYERR, UART_USR1_PARITYERR_get(usr1));
    SET_SRC(UART_IRQ_SRC_RTSD, UART_USR1_RTSD_get(usr1));
    SET_SRC(UART_IRQ_SRC_RTSF, UART_USR2_RTSF_get(usr2));
    SET_SRC(UART_IRQ_SRC_DTRF, UART_USR2_DTRF_get(usr2));
    SET_SRC(UART_IRQ_SRC_RIDELT, UART_USR2_RIDELT_get(usr2));
    SET_SRC(UART_IRQ_SRC_DCDDELT, UART_USR2_DCDDELT_get(usr2));
    SET_SRC(UART_IRQ_SRC_DTRD, UART_USR1_DTRD_get(usr1));
    SET_SRC(UART_IRQ_SRC_SAD, UART_USR1_SAD_get(usr1));

#undef SET_SRC

    return sources;
}

// Handlers
typedef void (*uart_irq_handler_t)(const driver_handle* h);
static void unhandled_irq_source(const driver_handle*)
{
    PANIC("unhandled");
}

// UART_IRQ_SRC_RRDY: Received data interrupt handler (with the threashold set
// to 1) adds the received data to the rx buffer
static void handle_RRDY_(const driver_handle* h)
{
#ifdef TEST
    if (UART_UTS_RXEMPTY_get(UART_UTS_read(h->base)))
        PANIC("RRDY irq arrived but no data was available");
#endif
    // Push all the uart fifo data into the driver ring buffer
    do {
        UartUrxdValue urxd = UART_URXD_read(h->base);
        uint8_t data = UART_URDX_RX_DATA_get(urxd);

        bool non_overwrite = uart_rxbuf_push(h, data);
        if (!non_overwrite)
            PANIC("Uart rx buffer overwrite"); // TODO: better handling of
                                               // overwrites
    } while (!UART_UTS_RXEMPTY_get(UART_UTS_read(h->base)));
}

// UART_IRQ_SRC_TRDY: Tx hardware fifo reached less or the stablished value , it
// tries to fill the buffer again with the data saved in the software driver tx
// buffer
static void handle_TRDY_(const driver_handle* h)
{
#ifdef TEST
    if (!UART_USR1_TRDY_get(UART_USR1_read(h->base)))
        PANIC("TRDY irq arrived but irq bit was not on");
#endif

    uint8_t data;
    bool txbuf_empty = false;
    while (!UART_UTS_TXFULL_get(UART_UTS_read(h->base))) {
        txbuf_empty = !uart_txbuf_pop(h, &data);

        if (txbuf_empty) {
            // disable the tx threashold irq, as there is
            // no more data available to send. It is enabled again when using
            // uart_putc
            uart_set_irq_state_(h, UART_IRQ_SRC_TRDY, false);

            break;
        }

        // Send the data
        UART_UTXD_write(h->base, data);
    }
}

static const uart_irq_handler_t UART_IRQ_SOURCE_HANDLERS_[] = {
    [UART_IRQ_SRC_RRDY] = handle_RRDY_,
    [UART_IRQ_SRC_RRDY + 1 ... UART_IRQ_SRC_TRDY - 1] = unhandled_irq_source,
    [UART_IRQ_SRC_TRDY] = handle_TRDY_,
    [UART_IRQ_SRC_TRDY + 1 ... UART_IRQ_SRC_COUNT - 1] = unhandled_irq_source,
};

void uart_handle_irq(const driver_handle* h)
{
    irqlocked()
    { // TODO: check if needed the lock
        bitfield32 source = uart_get_irq_sources(h);

        for (size_t i = UART_IRQ_SRC_START; i < UART_IRQ_SRC_COUNT; i++)
            if (bitfield_get(source, i))
                UART_IRQ_SOURCE_HANDLERS_[i](h);
    }
}

/*
 *  ------------
 *      INIT
 *  ------------
 */
void uart_init_stage0(const driver_handle* h)
{
#ifdef TEST
    uart_check_handle_(h);
#endif
    uart_reset(h);

    // 17.2.12.1 7357
    uintptr_t base = h->base;

    UartUcr1Value ucr1 = {0};
    UART_UCR1_UARTEN_set(&ucr1, true);
    UART_UCR1_IDEN_set(&ucr1, false);
    UART_UCR1_write(base, ucr1);

    UartUcr2Value ucr2 = {0};
    UART_UCR2_SRST_set(&ucr2, true);
    UART_UCR2_RXEN_set(&ucr2, true);
    UART_UCR2_TXEN_set(&ucr2, true);
    UART_UCR2_WS_set(&ucr2, true);
    UART_UCR2_IRTS_set(&ucr2, true);
    UART_UCR2_write(base, ucr2);

    UartUcr3Value ucr3 = {0};
    UART_UCR3_RXDMUXSEL_set(&ucr3, true);
    UART_UCR3_RXDSEN_set(&ucr3, false);
    UART_UCR3_AWAKEN_set(&ucr3, false);
    UART_UCR3_write(base, ucr3);

    UartUcr4Value ucr4 = {0};
    UART_UCR4_CTSTL_set(&ucr4, 32);
    UART_UCR4_write(base, ucr4);

    UartUfcrValue ufcr = {0};     // 0000 1010 0000 0001
    UART_UFCR_RXTL_set(&ufcr, 1); // RX fifo threshold interrupt 1
    UART_UFCR_TXTL_set(&ufcr, 4); // TX fifo threshold interrupt 1
    UART_UFCR_DCEDTE_set(&ufcr, false);
    UART_UFCR_RFDIV_set(&ufcr, UART_UFCR_RFDIV_DIV_BY_2);
    UART_UFCR_write(base, ufcr);

    UartUbirValue ubir = {0};
    UART_UBIR_INC_set(&ubir, 0xF);
    UART_UBIR_write(base, ubir);

    UartUbmrValue ubmr = {0};
    UART_UBMR_MOD_set(&ubmr, 0x68);
    UART_UBMR_write(base, ubmr);

    UartUmcrValue umcr = {0};
    UART_UMCR_write(base, umcr);

    // Flush rx fifo
    UartUrxdValue urxd = UART_URXD_read(base);
    while (!UART_UTS_RXEMPTY_get(UART_UTS_read(base)))
        UART_URDX_RX_DATA_get(urxd);

    uint32_t usr1_v = 0;
    for (size_t i = 0; i < 9; i++)
        usr1_v |= (0b1 << USR1_IRQ_W1C_BITS_[i]);

    uint32_t usr2_v = 0;
    for (size_t i = 0; i < 8; i++)
        usr2_v |= (0b1 << USR2_IRQ_W2C_BITS_[i]);

    UART_USR1_write(base, (UartUsr1Value) {.val = usr1_v});
    UART_USR2_write(base, (UartUsr2Value) {.val = usr2_v});
}

void uart_init_stage1(const driver_handle* h)
{
    uart_set_irq_state_(h, UART_IRQ_SRC_RRDY, true);
    uart_set_irq_state_(h, UART_IRQ_SRC_TRDY, true);
}

/*
 *  ------------
 *      put
 *  ------------
 */
term_out_result uart_putc_sync(const driver_handle* h, const char c)
{
#ifdef TEST
    uart_check_handle_(h);
#endif

    while (UART_tx_fifo_full_(h->base))
        asm volatile("nop");

    UART_UTXD_write(h->base, c);

    return TERM_OUT_RES_OK;
}


term_out_result uart_putc(const driver_handle* h, const char c)
{
    bool txbuf_full = !uart_txbuf_push(h, c);

    if (txbuf_full)
        return TERM_OUT_RES_NOT_TAKEN;

    // If TRDY is not enabled enable it
    if (!uart_get_irq_state_(h, UART_IRQ_SRC_TRDY))
        uart_set_irq_state_(h, UART_IRQ_SRC_TRDY, true);

    return TERM_OUT_RES_OK;
}


/*
 *  Uart early
 */

static p_uintptr_t early_base;
void uart_early_init(p_uintptr_t base)
{
    early_base = base;


    // software reset
    UART_UCR2_write(base, (UartUcr2Value) {.val = 0});
    while (!(UART_UCR2_read(base).val & 1))
        asm volatile("nop");


    UartUcr1Value ucr1 = {0};
    UART_UCR1_UARTEN_set(&ucr1, true);
    UART_UCR1_write(base, ucr1);

    UartUcr2Value ucr2 = {0};
    UART_UCR2_SRST_set(&ucr2, true);
    UART_UCR2_TXEN_set(&ucr2, true);
    UART_UCR2_WS_set(&ucr2, true);
    UART_UCR2_IRTS_set(&ucr2, true);
    UART_UCR2_write(base, ucr2);

    UartUcr3Value ucr3 = {0};
    UART_UCR3_RXDMUXSEL_set(&ucr3, true);
    UART_UCR3_write(base, ucr3);

    UartUcr4Value ucr4 = {0};
    UART_UCR4_CTSTL_set(&ucr4, 32);
    UART_UCR4_write(base, ucr4);

    UartUfcrValue ufcr = {0};     // 0000 1010 0000 0001
    UART_UFCR_RXTL_set(&ufcr, 0); // RX fifo threshold interrupt 1
    UART_UFCR_TXTL_set(&ufcr, 0); // TX fifo threshold interrupt 4
    UART_UFCR_DCEDTE_set(&ufcr, false);
    UART_UFCR_RFDIV_set(&ufcr, UART_UFCR_RFDIV_DIV_BY_2);
    UART_UFCR_write(base, ufcr);

    UartUbirValue ubir = {0};
    UART_UBIR_INC_set(&ubir, 0xF);
    UART_UBIR_write(base, ubir);

    UartUbmrValue ubmr = {0};
    UART_UBMR_MOD_set(&ubmr, 0x68);
    UART_UBMR_write(base, ubmr);

    UartUmcrValue umcr = {0};
    UART_UMCR_write(base, umcr);

    uint32_t usr1_v = 0;
    for (size_t i = 0; i < 9; i++)
        usr1_v |= (0b1 << USR1_IRQ_W1C_BITS_[i]);

    uint32_t usr2_v = 0;
    for (size_t i = 0; i < 8; i++)
        usr2_v |= (0b1 << USR2_IRQ_W2C_BITS_[i]);

    UART_USR1_write(base, (UartUsr1Value) {.val = usr1_v});
    UART_USR2_write(base, (UartUsr2Value) {.val = usr2_v});
}


term_out_result uart_putc_early(const char c)
{
    return uart_putc_sync(
        &(driver_handle) {
            .base = as_kva(early_base),
            .state = (void*)(1),
        },
        c);
}
