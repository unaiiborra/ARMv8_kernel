#include <drivers/uart/uart.h>
#include <drivers/uart/uart_raw.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/lock.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>



#define RX_FIFO_SZ 32
#define TX_FIFO_SZ 32


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
} irq_source_t;

// Saves which irqs are enabled or disabled, each bitfield represents each
// UART_ID
constexpr uint8_t USR1_IRQ_W1C_BITS[9] = {
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

constexpr uint8_t USR2_IRQ_W1C_BITS[8] = {
    1,
    2,
    4,
    7,
    8,
    11,
    12,
    15,
};


static int32_t imx8mp_uart_init(driver_handle_t handle)
{
    uintptr_t base = handle.base;

    UART_UCR2_write(base, (UartUcr2Value) {.val = 0});
    while (!(UART_UCR2_read(base).val & 1))
        asm volatile("nop");

    UartUcr1Value ucr1 = {0};
    UART_UCR1_UARTEN_set(&ucr1, true);
    UART_UCR1_write(base, ucr1);

    UartUcr2Value ucr2 = {0};
    UART_UCR2_SRST_set(&ucr2, true);
    UART_UCR2_TXEN_set(&ucr2, true);
    UART_UCR2_RXEN_set(&ucr2, true);
    UART_UCR2_WS_set(&ucr2, true);
    UART_UCR2_IRTS_set(&ucr2, true);
    UART_UCR2_write(base, ucr2);

    UartUcr3Value ucr3 = {0};
    UART_UCR3_RXDMUXSEL_set(&ucr3, true);
    UART_UCR3_write(base, ucr3);

    UartUcr4Value ucr4 = {0};
    UART_UCR4_CTSTL_set(&ucr4, 32);
    UART_UCR4_write(base, ucr4);

    UartUfcrValue ufcr = {0};
    UART_UFCR_DCEDTE_set(&ufcr, false);
    UART_UFCR_RFDIV_set(&ufcr, UART_UFCR_RFDIV_DIV_BY_2);
    UART_UFCR_write(base, ufcr);

    UartUmcrValue umcr = {0};
    UART_UMCR_write(base, umcr);

    // flush rx fifo
    while (!UART_UTS_RXEMPTY_get(UART_UTS_read(base)))
        UART_URXD_read(base);

    return 0;
}

static int32_t imx8mp_uart_exit(driver_handle_t handle)
{
    if (*handle.state != NULL) {
        IMX8MP_UART_OPS->irq_disable(handle);
    }

    UART_UCR1_write(handle.base, (UartUcr1Value) {0});

    return 0;
}

static int32_t imx8mp_uart_set_baud(
    driver_handle_t handle,
    uint32_t        baud,
    uint32_t        clock_hz)
{
    if (baud == 0)
        return -1;

    // UBIR as 0xF (increment of 16):
    // UBMR+1 = RefFreq / BaudRate
    // → UBMR = RefFreq / BaudRate - 1
    uint16_t ubmr = (clock_hz / baud);

    if (ubmr == 0)
        return -1;

    ubmr -= 1;

    UartUbirValue ubir = {0};
    UART_UBIR_INC_set(&ubir, 0xF);
    UART_UBIR_write(handle.base, ubir);

    UartUbmrValue ubmr_v = {0};
    UART_UBMR_MOD_set(&ubmr_v, ubmr);
    UART_UBMR_write(handle.base, ubmr_v);

    return 0;
}

static int32_t imx8mp_uart_rx_is_empty(driver_handle_t handle)
{
    UartUtsValue uts     = UART_UTS_read(handle.base);
    bool         rxempty = UART_UTS_RXEMPTY_get(uts);

    return rxempty ? 1 : 0;
}

static int32_t imx8mp_uart_tx_is_empty(driver_handle_t handle)
{
    UartUtsValue uts     = UART_UTS_read(handle.base);
    bool         txempty = UART_UTS_TXEMPTY_get(uts);

    return txempty ? 1 : 0;
}

static int32_t imx8mp_uart_rx_full(driver_handle_t handle)
{
    UartUtsValue uts    = UART_UTS_read(handle.base);
    bool         rxfull = UART_UTS_RXFULL_get(uts);

    return rxfull ? 1 : 0;
}

static int32_t imx8mp_uart_tx_full(driver_handle_t handle)
{
    UartUtsValue uts    = UART_UTS_read(handle.base);
    bool         txfull = UART_UTS_TXFULL_get(uts);

    return txfull ? 1 : 0;
}

static int32_t imx8mp_uart_getc(driver_handle_t handle)
{
    if (imx8mp_uart_rx_is_empty(handle) == 1) {
        return -1;
    }

    UartUrxdValue urxd = UART_URXD_read(handle.base);
    uint8_t       data = UART_URDX_RX_DATA_get(urxd);

    return (int32_t)data;
}

static int32_t imx8mp_uart_putc(driver_handle_t handle, char c)
{
    if (imx8mp_uart_tx_full(handle) == 1) {
        return -1;
    }

    UART_UTXD_write(handle.base, c);

    return 0;
}


typedef struct {
    spinlock_t lock;
    // for now it does not make a lot of sense as only 2 irqs
    // are used but for later implementations it will be useful
    bitfield32 irq_cache;
    void (*notify_rx_data_handler)(void* ctx);
    void (*notify_tx_data_handler)(void* ctx);
    void* notify_rx_data_ctx;
    void* notify_tx_data_ctx;
} uart_state_t;


static int32_t imx8mp_uart_irq_enable(driver_handle_t handle)
{
    *handle.state       = kmalloc(sizeof(uart_state_t));
    uart_state_t* state = *handle.state;

    *state = (uart_state_t) {
        .lock                   = SPINLOCK_INIT,
        .irq_cache              = 0,
        .notify_rx_data_handler = NULL,
        .notify_tx_data_handler = NULL,
        .notify_rx_data_ctx     = NULL,
        .notify_tx_data_ctx     = NULL,
    };

    // clean pending flags
    uint32_t usr1_v = 0;
    for (size_t i = 0; i < 9; i++)
        usr1_v |= (0b1 << USR1_IRQ_W1C_BITS[i]);

    uint32_t usr2_v = 0;
    for (size_t i = 0; i < 8; i++)
        usr2_v |= (0b1 << USR2_IRQ_W1C_BITS[i]);

    UART_USR1_write(handle.base, (UartUsr1Value) {.val = usr1_v});
    UART_USR2_write(handle.base, (UartUsr2Value) {.val = usr2_v});

    return 0;
}


static int32_t imx8mp_uart_irq_disable(driver_handle_t handle)
{
    uart_state_t* state = *handle.state;
    if (state == NULL)
        return 0;

    // disable irqs
    UartUcr1Value ucr1 = UART_UCR1_read(handle.base);
    ucr1.val           = ucr1.val & UARTEN_MASK; // only leave uarten enabled
    UART_UCR1_write(handle.base, ucr1);

    kfree(*handle.state);
    *handle.state = NULL;

    return 0;
}


static int32_t imx8mp_uart_irq_notify_rx(
    driver_handle_t handle,
    uint32_t        threshold,
    void (*handler)(void* ctx),
    void* ctx)
{
    uart_state_t* state = *handle.state;

    if (unlikely(state == NULL)) {
        return -1; // IRQ NOT enabled
    }

    if (unlikely(threshold > RX_FIFO_SZ)) {
        return -1; // threshold not valid
    }

    irq_spinlocked(&state->lock)
    {
        state->notify_rx_data_handler = handler;
        state->notify_rx_data_ctx     = ctx;

        if (handler != NULL) {
            // set threshold and enable irq
            UartUfcrValue ufcr = UART_UFCR_read(handle.base);
            UART_UFCR_RXTL_set(&ufcr, threshold);
            UART_UFCR_write(handle.base, ufcr);

            UartUcr1Value ucr1 = UART_UCR1_read(handle.base);
            UART_UCR1_RRDYEN_set(&ucr1, true);
            UART_UCR1_write(handle.base, ucr1);

            bitfield_set(state->irq_cache, UART_IRQ_SRC_RRDY, true);
        }
        else {
            // disable irq
            UartUcr1Value ucr1 = UART_UCR1_read(handle.base);
            UART_UCR1_RRDYEN_set(&ucr1, false);
            UART_UCR1_write(handle.base, ucr1);

            bitfield_set(state->irq_cache, UART_IRQ_SRC_RRDY, false);
        }
    }

    return 0;
}


static int32_t imx8mp_uart_irq_notify_tx(
    driver_handle_t handle,
    uint32_t        threshold,
    void (*handler)(void* ctx),
    void* ctx)
{
    uart_state_t* state = *handle.state;

    if (unlikely(state == NULL))
        return -1;

    if (unlikely(threshold > (uint32_t)TX_FIFO_SZ))
        return -1;

    irq_spinlocked(&state->lock)
    {
        state->notify_tx_data_handler = handler;
        state->notify_tx_data_ctx     = ctx;

        if (handler != NULL) {
            UartUfcrValue ufcr = UART_UFCR_read(handle.base);
            UART_UFCR_TXTL_set(&ufcr, threshold);
            UART_UFCR_write(handle.base, ufcr);

            UartUcr1Value ucr1 = UART_UCR1_read(handle.base);
            UART_UCR1_TRDYEN_set(&ucr1, true);
            UART_UCR1_write(handle.base, ucr1);

            bitfield_set(state->irq_cache, UART_IRQ_SRC_TRDY, true);
        }
        else {
            UartUcr1Value ucr1 = UART_UCR1_read(handle.base);
            UART_UCR1_TRDYEN_set(&ucr1, false);
            UART_UCR1_write(handle.base, ucr1);

            bitfield_set(state->irq_cache, UART_IRQ_SRC_TRDY, false);
        }
    }

    return 0;
}


typedef void (*uart_irq_handler2_t)(driver_handle_t handle);

static void handle_RRDY(driver_handle_t handle)
{
    uart_state_t* state = *handle.state;

    irqlock_t flags = _spin_lock_irqsave(&state->lock);

    // copy locked to have coherency between the two variables
    void (*notify_rx_data_handler)(void*) = state->notify_rx_data_handler;
    void* notify_rx_data_ctx              = state->notify_rx_data_ctx;

    spin_unlock_irqrestore(&state->lock, flags);

    // call while unlocked to allow for new imx8mp_uart_irq_notify_tx calls
    if (notify_rx_data_handler != NULL)
        notify_rx_data_handler(notify_rx_data_ctx);
}

static void handle_TRDY(driver_handle_t handle)
{
    uart_state_t* state = *handle.state;


    irqlock_t flags = _spin_lock_irqsave(&state->lock);

    // copy locked to have coherency between the two variables
    void (*notify_tx_data_handler)(void*) = state->notify_tx_data_handler;
    void* notify_tx_data_ctx              = state->notify_tx_data_ctx;

    spin_unlock_irqrestore(&state->lock, flags);

    // call while unlocked to allow for new imx8mp_uart_irq_notify_tx calls
    if (notify_tx_data_handler != NULL)
        notify_tx_data_handler(notify_tx_data_ctx);
}

static void handle_unhandled(driver_handle_t handle)
{
    (void)handle;
    PANIC();
}


static const uart_irq_handler2_t UART_IRQ_SOURCE_HANDLERS[] = {
    [UART_IRQ_SRC_RRDY]                                = handle_RRDY,
    [UART_IRQ_SRC_RRDY + 1 ... UART_IRQ_SRC_TRDY - 1]  = handle_unhandled,
    [UART_IRQ_SRC_TRDY]                                = handle_TRDY,
    [UART_IRQ_SRC_TRDY + 1 ... UART_IRQ_SRC_COUNT - 1] = handle_unhandled,
};


static void imx8mp_uart_irq_handle(driver_handle_t handle)
{
    uart_state_t* state = *handle.state;
    if (unlikely(state == NULL))
        return;

    UartUsr1Value usr1 = UART_USR1_read(handle.base);
    // UartUsr2Value usr2 = UART_USR2_read(handle.base);

    bitfield32 sources = 0;

#define SET_SRC(bit, status)                                                 \
    sources |= ((bitfield32)(((uint32_t)(status) &                           \
                              (uint32_t)bitfield_get(state->irq_cache, bit)) \
                             << (bit)))

    SET_SRC(UART_IRQ_SRC_RRDY, UART_USR1_RRDY_get(usr1));
    // SET_SRC(UART_IRQ_SRC_IDLE, UART_USR2_IDLE_get(usr2));
    // SET_SRC(UART_IRQ_SRC_RDR, UART_USR2_RDR_get(usr2));
    // SET_SRC(UART_IRQ_SRC_RXDS, UART_USR1_RXDS_get(usr1));
    // SET_SRC(UART_IRQ_SRC_AGTIM, UART_USR1_AGTIM_get(usr1));
    // SET_SRC(UART_IRQ_SRC_TXFE, UART_USR2_TXFE_get(usr2));
    SET_SRC(UART_IRQ_SRC_TRDY, UART_USR1_TRDY_get(usr1));
    // SET_SRC(UART_IRQ_SRC_TXDC, UART_USR2_TXDC_get(usr2));
    // SET_SRC(UART_IRQ_SRC_ORE, UART_USR2_ORE_get(usr2));
    // SET_SRC(UART_IRQ_SRC_BRCD, UART_USR2_BRCD_get(usr2));
    // SET_SRC(UART_IRQ_SRC_WAKE, UART_USR2_WAKE_get(usr2));
    // SET_SRC(UART_IRQ_SRC_ADET, UART_USR2_ADET_get(usr2));
    // SET_SRC(UART_IRQ_SRC_ACST, UART_USR2_ACST_get(usr2));
    // SET_SRC(UART_IRQ_SRC_ESC, UART_USR1_ESCF_get(usr1));
    // SET_SRC(UART_IRQ_SRC_IRINT, UART_USR2_IRINT_get(usr2));
    // SET_SRC(UART_IRQ_SRC_AIRINT, UART_USR1_AIRINT_get(usr1));
    // SET_SRC(UART_IRQ_SRC_AWAKE, UART_USR1_AWAKE_get(usr1));
    // SET_SRC(UART_IRQ_SRC_FRAERR, UART_USR1_FRAERR_get(usr1));
    // SET_SRC(UART_IRQ_SRC_PARITYERR, UART_USR1_PARITYERR_get(usr1));
    // SET_SRC(UART_IRQ_SRC_RTSD, UART_USR1_RTSD_get(usr1));
    // SET_SRC(UART_IRQ_SRC_RTSF, UART_USR2_RTSF_get(usr2));
    // SET_SRC(UART_IRQ_SRC_DTRF, UART_USR2_DTRF_get(usr2));
    // SET_SRC(UART_IRQ_SRC_RIDELT, UART_USR2_RIDELT_get(usr2));
    // SET_SRC(UART_IRQ_SRC_DCDDELT, UART_USR2_DCDDELT_get(usr2));
    // SET_SRC(UART_IRQ_SRC_DTRD, UART_USR1_DTRD_get(usr1));
    // SET_SRC(UART_IRQ_SRC_SAD, UART_USR1_SAD_get(usr1));

#undef SET_SRC

    for (irq_source_t i = UART_IRQ_SRC_START; i < UART_IRQ_SRC_COUNT; i++)
        if (bitfield_get(sources, i))
            UART_IRQ_SOURCE_HANDLERS[i](handle);
}


static const serial_ops_t OPS = {
    .init          = imx8mp_uart_init,
    .exit          = imx8mp_uart_exit,
    .set_baud      = imx8mp_uart_set_baud,
    .rx_is_empty   = imx8mp_uart_rx_is_empty,
    .tx_is_empty   = imx8mp_uart_tx_is_empty,
    .rx_full       = imx8mp_uart_rx_full,
    .tx_full       = imx8mp_uart_tx_full,
    .getc          = imx8mp_uart_getc,
    .putc          = imx8mp_uart_putc,
    .rx_size       = RX_FIFO_SZ,
    .tx_size       = TX_FIFO_SZ,
    .irq_enable    = imx8mp_uart_irq_enable,
    .irq_disable   = imx8mp_uart_irq_disable,
    .irq_notify_rx = imx8mp_uart_irq_notify_rx,
    .irq_notify_tx = imx8mp_uart_irq_notify_tx,
    .irq_handle    = imx8mp_uart_irq_handle,
};

const serial_ops_t* const IMX8MP_UART_OPS = &OPS;