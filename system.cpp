#include "system.h"

extern "C" char *sbrk(int32_t i);

void system_sleep(void)
{
    __DSB();
    __WFI();
}

uint32_t system_get_device_id(void)
{
	return DSU->DID.reg;
}

uint32_t system_get_reset_cause(void)
{
	return PM->RCAUSE.reg;
}

uint32_t system_get_free_memory(void)
{
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}

uint32_t system_deep_sleep(void) {
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    system_sleep();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    return 0;
}
