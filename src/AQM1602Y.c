#include "AQM1602Y.h"

void lcd_send_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x80, cmd};
    SERCOM1_I2C_Write(AQM1602Y_ADDR, buf, 2);
    delay_ms(1);
}

void AQM1602Y_Initialize()
{
    uint8_t data[9] = {0x38, 0x39, 0x14, 0x73, 0x56, 0x6c, 0x38, 0x0c, 0x01};
    nRST_Clear();
    delay_ms(1);
    nRST_Set();
    delay_ms(1);
    for (uint8_t i = 0; i < 9; i++)
    {
        lcd_send_cmd(data[i]);
    }
    AQM1602Y_InitArrowChars();
}

void AQM1602Y_Clear()
{
    lcd_send_cmd(0x01);
}

void AQM1602Y_Corsor_YX(uint8_t y, uint8_t x)
{
    x = x % 16;
    y = y % 2;
    uint8_t cmd = 0x80 | x | (y << 6);
    lcd_send_cmd(cmd);
}

void AQM1602Y_Print(char str[], uint8_t length)
{
    uint8_t buf[17] = {0x40, 0x00};
    uint8_t len = 1;
    for (int i = 0; i < length; i++)
    {
        if (str[i] == '\0')
            break;
        buf[i + 1] = str[i];
        len++;
    }
    SERCOM1_I2C_Write(AQM1602Y_ADDR, buf, len);
    delay_ms(2);
}

void AQM1602Y_RegisterCustomChar(uint8_t char_code, uint8_t pattern[8])
{
    lcd_send_cmd(0x40 | ((char_code & 0x07) << 3));

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t buf[2] = {0x40, pattern[i]};
        SERCOM1_I2C_Write(AQM1602Y_ADDR, buf, 2);
        delay_ms(1);
    }

    lcd_send_cmd(0x80);
}

void AQM1602Y_InitArrowChars()
{
    uint8_t up_arrow[8] = {
        0b00000100, // ....#....
        0b00001110, // ...###...
        0b00010101, // ..#.#.#..
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00000000  // .........
    };

    uint8_t down_arrow[8] = {
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00000100, // ....#....
        0b00010101, // ..#.#.#..
        0b00001110, // ...###...
        0b00000100, // ....#....
        0b00000000  // .........
    };

    AQM1602Y_RegisterCustomChar(1, up_arrow);
    AQM1602Y_RegisterCustomChar(2, down_arrow);
}