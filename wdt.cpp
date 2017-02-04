#include "wdt.h"

#include <sam.h>

#define WDT_GCLK    4

volatile bool wdt_early_warning_triggered = false;

void wdt_clear_early_warning(void) {
	WDT->INTFLAG.reg = WDT_INTFLAG_EW;
}

bool wdt_is_early_warning(void) {
	return WDT->INTFLAG.reg & WDT_INTFLAG_EW;
}

void wdt_sync() {
    while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);
}

bool wdt_initialized = false;

void wdt_initialize() {
    if (wdt_initialized) {
        return;
    }

    wdt_initialized = true;

    // Do one-time initialization of the watchdog timer.

    PM->APBAMASK.reg |= PM_APBAMASK_WDT;

    // Setup GCLK for the watchdog using:
    // - Generic clock generator 2 as the source for the watchdog clock
    // - Low power 32khz internal oscillator as the source for generic clock
    //   generator 2.
    // - Generic clock generator 2 divisor to 32 so it ticks roughly once a
    //   millisecond.

    // Set generic clock generator 2 divisor to 4 so the clock divisor is 32.
    // From the datasheet the clock divisor is calculated as:
    //   2^(divisor register value + 1)
    // A 32khz clock with a divisor of 32 will then generate a 1ms clock period.
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(WDT_GCLK) | GCLK_GENDIV_DIV(4);

    // Configure the GCLK module
    // GCLK_GENCTRL_GENEN, enable the specific GCLK module
    // GCLK_GENCTRL_SRC_OSCULP32K, set the source to the OSCULP32K
    // GCLK_GENCTRL_ID(X), specifies which GCLK we are configuring
    // GCLK_GENCTRL_DIVSEL, specify which prescalar mode we are using
    // Output from this module is 1khz (32khz / 32)
    // This register has to be written in a single operation.
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(WDT_GCLK) |
                        GCLK_GENCTRL_GENEN |
                        GCLK_GENCTRL_SRC_OSCULP32K |
                        GCLK_GENCTRL_DIVSEL;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

    // Configure the WDT clock
    // GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_WDT), specify the WDT clock
    // GCLK_CLKCTRL_GEN(WDT_GCLK), specify the source from the WDT_GCLK GCLK
    // This register has to be written in a single operation
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_WDT) |
                        GCLK_CLKCTRL_GEN(WDT_GCLK) |
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
}

uint8_t wdt_enable(uint8_t period) {
    wdt_initialize();

    // First disable the watchdog so its registers can be changed.
    WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
    wdt_sync();

    // Disable windowed mode
    WDT->CTRL.reg &= ~WDT_CTRL_WEN;
    wdt_sync();

    // Set the reset period to twice that of the
    // specified interrupt period
    WDT->CONFIG.reg = WDT_CONFIG_PER(period + 1);

    // Set the early warning as specified by the period
    WDT->EWCTRL.reg = WDT_EWCTRL_EWOFFSET(period);

    // Enable the WDT module
    WDT->CTRL.reg |= WDT_CTRL_ENABLE;
    wdt_sync();

    // Enable early warning interrupt
    WDT->INTENSET.reg = WDT_INTENSET_EW;

    // Enable interrupt vector for WDT
    // Priority is set to 0x00, the highest
    NVIC_EnableIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0x00);

    return 0;
}

void wdt_disable() {
    // Disable the WDT module
    WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
    while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);

    // Turn off the power to the WDT module
    PM->APBAMASK.reg &= ~PM_APBAMASK_WDT;
}

void wdt_checkin() {
    // Reset counter and wait for synchronisation
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);
}

bool wdt_read_early_warning() {
    bool triggered = wdt_early_warning_triggered;
    wdt_early_warning_triggered = false;
    return triggered;
}

void WDT_Handler(void) {
    wdt_early_warning_triggered = true;
    WDT->INTFLAG.reg = WDT_INTFLAG_EW;
}
