#ifndef AQM1602Y_H
#define AQM1602Y_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "definitions.h"
#include "delay.h"

#define AQM1602Y_ADDR 0x3E

    void AQM1602Y_Initialize();
    void AQM1602Y_Clear();
    void AQM1602Y_Corsor_YX(uint8_t y, uint8_t x);
    void AQM1602Y_Print(char str[], uint8_t length);
    void AQM1602Y_RegisterCustomChar(uint8_t char_code, uint8_t pattern[8]);
    void AQM1602Y_InitArrowChars();

#ifdef __cplusplus
}
#endif

#endif