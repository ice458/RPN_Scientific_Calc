#include "delay.h"

void delay_ms(uint32_t t) {
    uint32_t i;
    for (i = 0; i < t; i++) {
        TC1_TimerStart();
        while (!TC1_TimerPeriodHasExpired());
        TC1_TimerStop();
        TC1_REGS->COUNT16.TC_INTFLAG = (uint8_t)TC_INTFLAG_Msk;
    }
}