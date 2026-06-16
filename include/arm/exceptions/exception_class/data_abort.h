#pragma once

#include <stdint.h>

typedef struct {
    uint32_t DFSC;   /* [5:0]   */
    uint32_t WnR;    /* [6]     */
    uint32_t S1PTW;  /* [7]     */
    uint32_t CM;     /* [8]     */
    uint32_t EA;     /* [9]     */
    uint32_t FnV;    /* [10]    */
    uint32_t b12_11; /* [12:11] */
    uint32_t b14;    /* [14]    */
    uint32_t b15;    /* [15]    */
    uint32_t b20_16; /* [20:16] */
    uint32_t SSE;    /* [21]    */
    uint32_t SAS;    /* [23:22] */
    uint32_t ISV;    /* [24]    */
} data_abort_iss;


static inline data_abort_iss data_abort_iss_new(uint32_t iss)
{
    data_abort_iss d;

    d.DFSC   = iss & 0x3fu;         /* bits [5:0]   */
    d.WnR    = (iss >> 6) & 0x1u;   /* bit  [6]     */
    d.S1PTW  = (iss >> 7) & 0x1u;   /* bit  [7]     */
    d.CM     = (iss >> 8) & 0x1u;   /* bit  [8]     */
    d.EA     = (iss >> 9) & 0x1u;   /* bit  [9]     */
    d.FnV    = (iss >> 10) & 0x1u;  /* bit  [10]    */
    d.b12_11 = (iss >> 11) & 0x3u;  /* bits [12:11] */
    d.b14    = (iss >> 14) & 0x1u;  /* bit  [14]    */
    d.b15    = (iss >> 15) & 0x1u;  /* bit  [15]    */
    d.b20_16 = (iss >> 16) & 0x1fu; /* bits [20:16] */
    d.SSE    = (iss >> 21) & 0x1u;  /* bit  [21]    */
    d.SAS    = (iss >> 22) & 0x3u;  /* bits [23:22] */
    d.ISV    = (iss >> 24) & 0x1u;  /* bit  [24]    */

    return d;
}
