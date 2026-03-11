#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <drivers/uart/raw/uart_onems.h>
#include <drivers/uart/raw/uart_ubir.h>
#include <drivers/uart/raw/uart_ubmr.h>
#include <drivers/uart/raw/uart_ubrc.h>
#include <drivers/uart/raw/uart_ucr1.h>
#include <drivers/uart/raw/uart_ucr2.h>
#include <drivers/uart/raw/uart_ucr3.h>
#include <drivers/uart/raw/uart_ucr4.h>
#include <drivers/uart/raw/uart_uesc.h>
#include <drivers/uart/raw/uart_ufcr.h>
#include <drivers/uart/raw/uart_umcr.h>
#include <drivers/uart/raw/uart_urxd.h>
#include <drivers/uart/raw/uart_usr1.h>
#include <drivers/uart/raw/uart_usr2.h>
#include <drivers/uart/raw/uart_utim.h>
#include <drivers/uart/raw/uart_uts.h>
#include <drivers/uart/raw/uart_utxd.h>


extern const uint8_t USR1_IRQ_W1C_BITS_[9];
extern const uint8_t USR2_IRQ_W2C_BITS_[8];
