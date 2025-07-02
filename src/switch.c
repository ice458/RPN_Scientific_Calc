#include "switch.h"

volatile uint8_t key = 0;

void dynamic()
{
    static uint8_t sel = 0;
    static uint8_t counter = 0;
    uint8_t tmp;

    switch (sel)
    {
    case 0:
        D0_Set();
        break;
    case 1:
        D1_Set();
        break;
    case 2:
        D2_Set();
        break;
    case 3:
        D3_Set();
        break;
    case 4:
        D4_Set();
        break;
    case 5:
        D5_Set();
        break;
    case 6:
        D6_Set();
        break;
    }

    tmp = (PORT_GroupRead(PORT_GROUP_0) >> 18U) & 0b11111U;
    // キーが押されたとき
    if (tmp != 0)
    {
        key = tmp + (sel << 5);
        counter = 0;
    }
    // キーが押されていないとき
    else
    {
        counter++;
        // すべてのキーが押されていないとき
        if (counter >= 7)
        {
            counter = 0;
            key = 0;
        }
    }

    switch (sel)
    {
    case 0:
        D0_Clear();
        break;
    case 1:
        D1_Clear();
        break;
    case 2:
        D2_Clear();
        break;
    case 3:
        D3_Clear();
        break;
    case 4:
        D4_Clear();
        break;
    case 5:
        D5_Clear();
        break;
    case 6:
        D6_Clear();
        break;
    }
    sel++;
    if (sel >= 7)
    {
        sel = 0;
    }
}

uint8_t key_read()
{
    return key;
}

void clear_key()
{
    key = 0;
}

void init_sw()
{
    // スイッチ読み取り割り込み 周期10msec
    TC2_TimerCallbackRegister(dynamic, (uintptr_t)NULL);
    TC2_TimerStart();
}
