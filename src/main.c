#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "definitions.h"

#include "AQM1602Y.h"
#include "switch.h"
#include "decimal_float.h"
#include "delay.h"

// *****************************************************************************
// *****************************************************************************
// スタック操作関連
// *****************************************************************************
// *****************************************************************************
df_t stack[4];

void stack_init()
{
    int_to_df(0, &stack[0]);
    int_to_df(0, &stack[1]);
    int_to_df(0, &stack[2]);
    int_to_df(0, &stack[3]);
}

void stack_push()
{
    stack[3] = stack[2];
    stack[2] = stack[1];
    stack[1] = stack[0];
}

void stack_pop()
{
    stack[0] = stack[1];
    stack[1] = stack[2];
    stack[2] = stack[3];
}

void stack_swap()
{
    df_swap(&stack[0], &stack[1]);
}

void stack_roll_up()
{
    df_t tmp = stack[3];
    stack[3] = stack[2];
    stack[2] = stack[1];
    stack[1] = stack[0];
    stack[0] = tmp;
}

void stack_roll_down()
{
    df_t tmp = stack[0];
    stack[0] = stack[1];
    stack[1] = stack[2];
    stack[2] = stack[3];
    stack[3] = tmp;
}

// *****************************************************************************
// *****************************************************************************
// 設定の読み書き
// *****************************************************************************
// *****************************************************************************

uint32_t poweron_setting[8];
// flashから読み出す
void read_config()
{
    uint32_t buf[8];
    NVMCTRL_Read(buf, 8, NVMCTRL_USERROW_START_ADDRESS);
    if (buf[0] & 1)
    {
        set_df_angle_mode(DF_ANGLE_MODE_RAD);
        poweron_setting[0] = 1;
    }
    else
    {
        set_df_angle_mode(DF_ANGLE_MODE_DEG);
        poweron_setting[0] = 0;
    }
    if (buf[1] & 1)
    {
        set_df_string_mode(DF_STRING_MODE_SCIENTIFIC);
        poweron_setting[1] = 1;
    }
    else
    {
        set_df_string_mode(DF_STRING_MODE_ENGINEERING);
        poweron_setting[1] = 0;
    }
}
// flashに書き込む
void write_config()
{
    uint32_t buf[8] = {0};
    switch (get_df_angle_mode())
    {
    case DF_ANGLE_MODE_DEG:
        buf[0] = 0;
        break;
    case DF_ANGLE_MODE_RAD:
        buf[0] = 1;
        break;
    default:
        break;
    }
    switch (get_df_string_mode())
    {
    case DF_STRING_MODE_ENGINEERING:
        buf[1] = 0;
        break;
    case DF_STRING_MODE_SCIENTIFIC:
        buf[1] = 1;
        break;
    default:
        break;
    }
    
    if(poweron_setting[0] != buf[0] || poweron_setting[1] != buf[1])
    {
        NVMCTRL_USER_ROW_RowErase(NVMCTRL_USERROW_START_ADDRESS);
        while (NVMCTRL_IsBusy())
            ;
        NVMCTRL_USER_ROW_PageWrite(buf, NVMCTRL_USERROW_START_ADDRESS);
        while (NVMCTRL_IsBusy())
            ;
    }
}

// 各種設定画面の表示
void config(bool mode_flag, bool disp_flag)
{
    AQM1602Y_Clear();
    // 表示文字列のバッファ
    char str[17];
    // 演算用一時変数
    df_t tmp;
    // キーナンバー
    uint8_t key = 254;      // ダミー初期値
    uint8_t last_key = 255; // ダミー初期値
    // キーが押されたかどうかのフラグ
    bool is_pushed = false;

    while (1)
    {
        // クロック周波数を落とす
        // GCLK0をOSCULP32Kに変更
        GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(3U) | GCLK_GENCTRL_GENEN_Msk;
        while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
            ;
        // DFLLをOFF
        OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE(false);
        // キー読み取りループ
        while (1)
        {
            if (last_key == 255)
            {
                last_key = 0;
                break;
            }
            key = key_read();
            if (key != last_key)
            {
                last_key = key;
                break;
            }
        }
        // オートパワーセーブ用タイマリセット
        TC0_Timer16bitCounterSet(0);

        // クロック周波数を上げる
        // DFLLをON
        OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE_Msk;
        while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) != OSCCTRL_STATUS_DFLLRDY_Msk)
            ;
        // GCLK0をDFLL48Mに変更
        GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(7U) | GCLK_GENCTRL_GENEN_Msk;
        while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
            ;

        // キー押下
        is_pushed = true;

        // キー解釈
        switch (key)
        {
        case KENTER:
            return;
            break;
        case K1:
            if (mode_flag == true)
            {
                set_df_angle_mode(DF_ANGLE_MODE_DEG);
            }
            else if (disp_flag == true)
            {
                set_df_string_mode(DF_STRING_MODE_ENGINEERING);
            }
            break;
        case K2:
            if (mode_flag == true)
            {
                set_df_angle_mode(DF_ANGLE_MODE_RAD);
            }
            else if (disp_flag == true)
            {
                set_df_string_mode(DF_STRING_MODE_SCIENTIFIC);
            }
            break;
        case 0:
            is_pushed = false;
            break;
        default:
            break;
        }

        // 表示処理
        if (is_pushed == true)
        {
            if (mode_flag == true)
            {
                AQM1602Y_Corsor_YX(1, 0);
                AQM1602Y_Print("  1.DEG  2.RAD  ", 16);
                switch (get_df_angle_mode())
                {
                case DF_ANGLE_MODE_DEG:
                    AQM1602Y_Corsor_YX(1, 1);
                    AQM1602Y_Print("*", 1);
                    break;
                case DF_ANGLE_MODE_RAD:
                    AQM1602Y_Corsor_YX(1, 8);
                    AQM1602Y_Print("*", 1);
                    break;
                default:
                    break;
                }
            }
            else if (disp_flag == true)
            {
                AQM1602Y_Corsor_YX(1, 0);
                AQM1602Y_Print("  1.ENG  2.SCI  ", 16);
                switch (get_df_string_mode())
                {
                case DF_STRING_MODE_ENGINEERING:
                    AQM1602Y_Corsor_YX(1, 1);
                    AQM1602Y_Print("*", 1);
                    break;
                case DF_STRING_MODE_SCIENTIFIC:
                    AQM1602Y_Corsor_YX(1, 8);
                    AQM1602Y_Print("*", 1);
                    break;
                default:
                    break;
                }

                // Xレジスタの表示
                AQM1602Y_Corsor_YX(0, 0);
                AQM1602Y_Print("                ", 16);
                df_round(&stack[0], &tmp);
                df_to_string(&tmp, str);
                if (stack[0].sign == 0)
                {
                    AQM1602Y_Corsor_YX(0, 1);
                }
                else
                {
                    AQM1602Y_Corsor_YX(0, 0);
                }
                AQM1602Y_Print(str, 16);
            }
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// オートパワーダウン
// *****************************************************************************
// *****************************************************************************

void pdown()
{
    AQM1602Y_Corsor_YX(0, 0);
    AQM1602Y_Print("                ", 16);
    AQM1602Y_Corsor_YX(1, 0);
    AQM1602Y_Print("    See you!    ", 16);
    write_config();
    delay_ms(500);
    POWER_EN_Clear();
}

// *****************************************************************************
// *****************************************************************************
// main
// *****************************************************************************
// *****************************************************************************

int main(void)
{
    SYS_Initialize(NULL);

    // 電源ON状態をラッチ
    POWER_EN_Set();

    // オートパワーセーブ用タイマ設定
    TC0_TimerCallbackRegister(pdown, (uintptr_t)NULL);
    TC0_TimerStart();

    // LCDの初期化
    AQM1602Y_Initialize();

    // キー読み取り機能の初期化
    init_sw();

    // 起動メッセージ表示
    AQM1602Y_Corsor_YX(1, 0);
    AQM1602Y_Print("     Hello!", 16);
    read_config();
    delay_ms(300);

    // スタックの初期化
    stack_init();
    // 表示文字列のバッファ
    char str[17];
    // 入力文字列のバッファ
#define max_inputval_length 18
    char inputval[max_inputval_length] = {0};
    // 入力された文字列の長さ
    uint8_t inputval_len = 0;
    // 指数表記のEの位置
    uint8_t pos_E = 0;
    // 小数点の位置
    uint8_t pos_dot = 0;
    // 仮数の符号(falseで正)
    bool pri_m = false;
    // 指数の符号(falseで正)
    bool pri_e = false;
    // 演算後に数値入力でpushするためのフラグ
    bool push_flag = false;

    // キーナンバー
    uint8_t key = 254;      // ダミー初期値
    uint8_t last_key = 255; // ダミー初期値

    // シフトキーのフラグ
    bool shift = false;
    // キーが押されたかどうかのフラグ
    bool is_pushed = false;
    // 演算用一時変数
    df_t tmp;
    // 直前のXレジスタの値
    df_t last_x;
    // モード設定表示フラグ
    bool mode_flag = false;
    // 表示設定表示フラグ
    bool disp_flag = false;
    while (1)
    {
        // クロック周波数を落とす
        // GCLK0をOSCULP32Kに変更
        GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(3U) | GCLK_GENCTRL_GENEN_Msk;
        while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
            ;
        // DFLLをOFF
        OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE(false);

        // キー読み取りループ
        while (1)
        {
            if (last_key == 255)
            {
                last_key = 0;
                break;
            }
            key = key_read();
            if (key != last_key)
            {
                last_key = key;
                break;
            }
        }
        // オートパワーセーブ用タイマリセット
        TC0_Timer16bitCounterSet(0);

        // クロック周波数を上げる
        // DFLLをON
        OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE_Msk;
        while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) != OSCCTRL_STATUS_DFLLRDY_Msk)
            ;
        // GCLK0をDFLL48Mに変更
        GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(7U) | GCLK_GENCTRL_GENEN_Msk;
        while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
            ;

        // キー押下
        is_pushed = true;

        // キー解釈
        switch (key)
        {
        case KSHIFT:
            shift ^= true;
            break;
        case KDEL:
            if (shift == true)
            {
                AQM1602Y_Corsor_YX(0, 0);
                AQM1602Y_Print("                ", 16);
                AQM1602Y_Corsor_YX(1, 0);
                AQM1602Y_Print("    See you!    ", 16);
                write_config();
                delay_ms(500);
                POWER_EN_Clear();
            }
            else
            {
                push_flag = false;
                if (0 < inputval_len && inputval_len < max_inputval_length)
                {
                    inputval_len--;
                    inputval[inputval_len] = '\0';
                    if (pos_E == inputval_len)
                    {
                        pos_E = 0;
                    }
                    if (pos_dot == inputval_len)
                    {
                        pos_dot = 0;
                    }
                    if(inputval_len == 0)
                    {
                        pri_m = false;
                    }
                }
                else
                {
                    for (uint8_t i = 0; i < max_inputval_length; i++)
                    {
                        inputval[i] = '\0';
                    }
                }
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K0:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '0';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K1:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '1';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K2:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '2';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K3:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '3';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K4:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '4';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K5:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '5';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K6:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '6';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K7:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '7';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K8:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '8';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K9:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (inputval_len < max_inputval_length)
            {
                inputval[inputval_len] = '9';
                inputval_len++;
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case KDOT:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (shift == true)
            {
                stack[0] = df_e();
                df_round(&stack[0], &tmp);
                df_to_string(&tmp, inputval);

                pos_E = 11;
                pos_dot = 1;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
                push_flag = true;
                break;
            }
            else
            {
                if (pos_dot == 0 && pos_E == 0 && inputval_len != 0)
                {
                    if (inputval_len < max_inputval_length)
                    {
                        inputval[inputval_len] = '.';
                        pos_dot = inputval_len;
                        inputval_len++;
                    }
                }
                else if (pos_dot == 0 && pos_E == 0 && inputval_len == 0)
                {
                    if (inputval_len < max_inputval_length)
                    {
                        inputval[inputval_len] = '0';
                        inputval_len++;
                        inputval[inputval_len] = '.';
                        pos_dot = inputval_len;
                        inputval_len++;
                    }
                }
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case KE:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (shift == true)
            {
                stack[0] = df_pi();
                df_round(&stack[0], &tmp);
                df_to_string(&tmp, inputval);
                pos_E = 11;
                pos_dot = 1;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
                push_flag = true;
                break;
            }
            else
            {
                if (pos_E == 0 && inputval_len != 0)
                {
                    if (inputval_len < max_inputval_length)
                    {
                        inputval[inputval_len] = 'E';
                        pos_E = inputval_len;
                        inputval_len++;
                    }
                }
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case KPRI:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (pos_E == 0)
            {
                if (pri_m == false) // 負にする時
                {
                    pri_m = true;
                    inputval_len++;
                    pos_dot = (pos_dot == 0) ? 0 : pos_dot + 1;
                    for (int8_t i = max_inputval_length - 2; i >= 0; i--)
                    {
                        inputval[i + 1] = inputval[i];
                    }
                    inputval[0] = '-';
                }
                else
                {
                    pri_m = false;
                    inputval_len--;
                    pos_dot = (pos_dot == 0) ? 0 : pos_dot - 1;
                    for (int8_t i = 0; i < max_inputval_length - 2; i++)
                    {
                        inputval[i] = inputval[i + 1];
                    }
                    inputval[inputval_len] = '\0';
                }
            }
            else // 指数部の符号
            {
                if (pri_e == false) // 負にする時
                {
                    pri_e = true;
                    inputval_len++;
                    for (int8_t i = max_inputval_length - 2; i > pos_E; i--)
                    {
                        inputval[i + 1] = inputval[i];
                    }
                    inputval[pos_E + 1] = '-';
                }
                else
                {
                    pri_e = false;
                    inputval_len--;
                    for (int8_t i = pos_E + 1; i < max_inputval_length - 2; i++)
                    {
                        inputval[i] = inputval[i + 1];
                    }
                    inputval[inputval_len] = '\0';
                }
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case KENTER:
            tmp = stack[0];
            stack_push();
            stack[0] = tmp;

            for (uint8_t i = 0; i < max_inputval_length; i++)
            {
                inputval[i] = '\0';
            }

            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            break;
        case KR:
            if (shift == true)
            {
                stack_roll_up();
            }
            else
            {
                stack_roll_down();
            }
            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            push_flag = true;
            break;
        case KSWAP:
            if (shift == true)
            {
                push_flag = false;
                stack_push();
                for (uint8_t i = 0; i < max_inputval_length; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = last_x;
            }
            else
            {
                stack_swap();
            }
            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            push_flag = true;
            break;
        case KADD:
            last_x = stack[0];
            df_add(&stack[1], &stack[0], &tmp);
            stack_pop();
            stack[0] = tmp;

            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            push_flag = true;
            break;
        case KSUB:
            last_x = stack[0];
            df_sub(&stack[1], &stack[0], &tmp);
            stack_pop();
            stack[0] = tmp;

            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            push_flag = true;
            break;
        case KMUL:
            last_x = stack[0];
            if (shift == true)
            {
                df_factorial(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_mul(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;

                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KDIV:
            last_x = stack[0];
            if (shift == true)
            {
                df_inv(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_div(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;

                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KSIN:
            last_x = stack[0];
            if (shift == true)
            {
                df_asin(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_sin(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KCOS:
            last_x = stack[0];
            if (shift == true)
            {
                df_acos(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_cos(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KTAN:
            last_x = stack[0];
            if (shift == true)
            {
                df_atan(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_tan(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KSQRT:
            last_x = stack[0];
            if (shift == true)
            {
                df_cbrt(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_sqrt(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KPOW2:
            last_x = stack[0];
            if (shift == true)
            {
                df_mul(&stack[0], &stack[0], &tmp);
                df_mul(&stack[0], &tmp, &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_mul(&stack[0], &stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KPOW:
            last_x = stack[0];
            if (shift == true)
            {
                df_nth_root(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_pow(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KLOG:
            last_x = stack[0];
            if (shift == true)
            {
                int_to_df(10, &tmp);
                df_pow(&tmp, &stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_log10(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KLN:
            last_x = stack[0];
            if (shift == true)
            {
                df_exp(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_ln(&stack[0], &tmp);
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KMoS:
            last_x = stack[0];
            df_mul_over_sum(&stack[1], &stack[0], &tmp);
            stack_pop();
            stack[0] = tmp;
            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;
            push_flag = true;
            break;
        case KRC:
            last_x = stack[0];
            if (shift == true)
            {
                df_fc_lc(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            else
            {
                df_fc_rc(&stack[1], &stack[0], &tmp);
                stack_pop();
                stack[0] = tmp;
                pos_E = 0;
                pos_dot = 0;
                pri_m = false;
                pri_e = false;
                inputval_len = 0;
                shift = false;
            }
            push_flag = true;
            break;
        case KLOGXY:
            last_x = stack[0];
            df_log(&stack[0], &stack[1], &tmp);
            stack_pop();
            stack[0] = tmp;
            pos_E = 0;
            pos_dot = 0;
            pri_m = false;
            pri_e = false;
            inputval_len = 0;
            shift = false;

            push_flag = true;
            break;
        case KF1:
            if (shift == true)
            {
                mode_flag = true;
                disp_flag = false;
                shift = false;
                inputval_len = 0;
            }
            else
            {
                // 任意関数1
            }
            break;
        case KF2:
            if (shift == true)
            {
                disp_flag = true;
                mode_flag = false;
                shift = false;
                inputval_len = 0;
            }
            else
            {
                // 任意関数2
            }
            break;

        case 0:
            is_pushed = false;
            break;
        default:
            break;
        }

        // 表示処理
        if (is_pushed == true)
        {
            if (mode_flag == true || disp_flag == true)
            {
                config(mode_flag, disp_flag);
                mode_flag = false;
                disp_flag = false;
                last_key = KENTER;
            }

            // シフト記号の削除
            if (shift == false)
            {
                AQM1602Y_Corsor_YX(0, 15);
                AQM1602Y_Print(" ", 1);
            }

            // Yレジスタの表示
            AQM1602Y_Corsor_YX(0, 0);
            AQM1602Y_Print("                ", 16);
            df_round(&stack[1], &tmp);
            if (tmp.exponent <= -100 || tmp.exponent >= 100)
            {
                int_to_df(0, &stack[1]);
            }
            df_to_string(&tmp, str);
            if (stack[1].sign == 0)
            {
                AQM1602Y_Corsor_YX(0, 1);
            }
            else
            {
                AQM1602Y_Corsor_YX(0, 0);
            }
            AQM1602Y_Print(str, 16);

            // シフト記号の表示
            if (shift == true)
            {
                AQM1602Y_Corsor_YX(0, 15);
                AQM1602Y_Print("s", 1);
            }

            // 入力中の文字の表示
            if (inputval_len > 0)
            {
                AQM1602Y_Corsor_YX(1, 0);
                AQM1602Y_Print("                ", 16);
                if (pri_m == false)
                {
                    AQM1602Y_Corsor_YX(1, 1);
                }
                else
                {
                    AQM1602Y_Corsor_YX(1, 0);
                }
                AQM1602Y_Print(inputval, 16);
            }
            // Xレジスタの表示
            else
            {
                AQM1602Y_Corsor_YX(1, 0);
                AQM1602Y_Print("                ", 16);
                df_round(&stack[0], &tmp);
                df_to_string(&tmp, str);
                if (strcmp(str, "OverFlow") == 0 || strcmp(str, "ERROR") == 0)
                {
                    int_to_df(0, &stack[0]);
                    push_flag = false;
                }
                if (stack[0].sign == 0)
                {
                    AQM1602Y_Corsor_YX(1, 1);
                }
                else
                {
                    AQM1602Y_Corsor_YX(1, 0);
                }
                AQM1602Y_Print(str, 16);
            }
        }

        // I2CはDFLL48Mをクロック源としている。
        // I2C通信中にクロックを無効にしないようにするため待機する
        delay_ms(5);
    }
    return (EXIT_FAILURE);
}
