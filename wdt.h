#ifndef WDT_H_INCLUDED
#define WDT_H_INCLUDED

#include <Arduino.h>

enum wdt_period {
    WDT_PERIOD_1DIV64 = 1,   // 16 cycles   = 1/64s
    WDT_PERIOD_1DIV32 = 2,   // 32 cycles   = 1/32s
    WDT_PERIOD_1DIV16 = 3,   // 64 cycles   = 1/16s
    WDT_PERIOD_1DIV8  = 4,   // 128 cycles  = 1/8s
    WDT_PERIOD_1DIV4  = 5,   // 256 cycles  = 1/4s
    WDT_PERIOD_1DIV2  = 6,   // 512 cycles  = 1/2s
    WDT_PERIOD_1X     = 7,   // 1024 cycles = 1s
    WDT_PERIOD_2X     = 8,   // 2048 cycles = 2s
    WDT_PERIOD_4X     = 9,   // 4096 cycles = 4s
    WDT_PERIOD_8X     = 10   // 8192 cycles = 8s
};

void wdt_clear_early_warning(void);

bool wdt_is_early_warning(void);

void wdt_sync();

uint8_t wdt_enable(uint8_t period);

void wdt_disable();

void wdt_checkin();

bool wdt_read_early_warning();

#endif
