#pragma once
#include <kernel/devices/device.h>
#include <kernel/io/term.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UART_TX_BUF_SIZE 8192
#define UART_RX_BUF_SIZE 1024


typedef struct {
    puintptr_t early_base;
    bitfield32 irq_status;

    _Alignas(64) struct {
        bool    overwrite;
        size_t  head;
        size_t  tail;
        uint8_t buf[UART_TX_BUF_SIZE];
    } tx;

    _Alignas(64) struct {
        bool    overwrite;
        size_t  head;
        size_t  tail;
        uint8_t buf[UART_RX_BUF_SIZE];
    } rx;
} uart_state;


/// waits until tx fifo is empty
void uart_tx_empty_barrier(const driver_handle* h);

/*
 *  Early init features
 */
void uart_early_init(puintptr_t base);

term_out_result uart_putc_early(const char c);


/*
 *  Full features
 */
void uart_reset(const driver_handle* h);

// Pre IRQ initialization
void uart_init_stage0(const driver_handle* h);

// Post IRQ initialization
void uart_init_stage1(const driver_handle* h);

bool uart_read(const driver_handle* h, uint8_t* data);

// The kernel should call this fn
void uart_handle_irq(const driver_handle* h);


term_out_result uart_putc_sync(const driver_handle* h, const char c);
term_out_result uart_putc(const driver_handle* h, const char c);


static inline size_t uart_tx_capacity()
{
    return UART_TX_BUF_SIZE;
}

static inline size_t uart_rx_capacity()
{
    return UART_RX_BUF_SIZE;
}

extern size_t uart_tx_len(const driver_handle* h);
extern size_t uart_rx_len(const driver_handle* h);
