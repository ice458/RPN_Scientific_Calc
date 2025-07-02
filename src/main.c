#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "definitions.h"

#include "AQM1602Y.h"
#include "switch.h"
#include "decimal_float.h"
#include "delay.h"

// *****************************************************************************
// *****************************************************************************
// 定数定義
// *****************************************************************************
// *****************************************************************************
#define MAX_INPUTVAL_LENGTH 18
#define CONFIG_BUFFER_SIZE 8
#define DISPLAY_WIDTH 16
#define STACK_SIZE 4
#define POWER_SAVE_TIMEOUT_MS 600000 // 10分

// *****************************************************************************
// *****************************************************************************
// スタック操作関連
// *****************************************************************************
// *****************************************************************************
df_t stack[STACK_SIZE];

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
// ヘルパー関数
// *****************************************************************************
// *****************************************************************************

// 入力値をクリアする
void clear_input_state(char *inputval, uint8_t *inputval_len, uint8_t *pos_E,
                       uint8_t *pos_dot, bool *pri_m, bool *pri_e)
{
    for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
    {
        inputval[i] = '\0';
    }
    *inputval_len = 0;
    *pos_E = 0;
    *pos_dot = 0;
    *pri_m = false;
    *pri_e = false;
}

// 数字入力共通関数
void handle_digit_input(char digit, char *inputval, uint8_t *inputval_len,
                        bool *push_flag, df_t *stack, uint8_t *pos_E,
                        uint8_t *pos_dot, bool *pri_m, bool *pri_e)
{
    if (*push_flag == true)
    {
        *push_flag = false;
        df_t tmp = stack[0];
        stack_push();
        clear_input_state(inputval, inputval_len, pos_E, pos_dot, pri_m, pri_e);
        stack[0] = tmp;
    }
    if (*inputval_len < MAX_INPUTVAL_LENGTH)
    {
        inputval[*inputval_len] = digit;
        (*inputval_len)++;
    }
    string_to_df(inputval, &stack[0]);
}

// 演算後共通処理
void after_operation(char *inputval, uint8_t *inputval_len, uint8_t *pos_E,
                     uint8_t *pos_dot, bool *pri_m, bool *pri_e,
                     bool *shift, bool *push_flag)
{
    clear_input_state(inputval, inputval_len, pos_E, pos_dot, pri_m, pri_e);
    *shift = false;
    *push_flag = true;
}

// クロック周波数制御
void set_low_frequency_clock()
{
    // GCLK0をOSCULP32Kに変更
    GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(3U) | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
        ;
    // DFLLをOFF
    OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE(false);
}

void set_high_frequency_clock()
{
    // DFLLをON
    OSCCTRL_REGS->OSCCTRL_DFLLCTRL = OSCCTRL_DFLLCTRL_ENABLE_Msk;
    while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) != OSCCTRL_STATUS_DFLLRDY_Msk)
        ;
    // GCLK0をDFLL48Mに変更
    GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1U) | GCLK_GENCTRL_SRC(7U) | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) == GCLK_SYNCBUSY_GENCTRL0_Msk)
        ;
}

// *****************************************************************************
// *****************************************************************************
// 設定読み書き
// *****************************************************************************
// *****************************************************************************
uint32_t poweron_setting[CONFIG_BUFFER_SIZE];

// flashから設定を読み込む
void read_config()
{
    uint32_t buf[CONFIG_BUFFER_SIZE];
    NVMCTRL_Read(buf, CONFIG_BUFFER_SIZE, NVMCTRL_USERROW_START_ADDRESS);
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
    uint32_t buf[CONFIG_BUFFER_SIZE] = {0};
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

    if (poweron_setting[0] != buf[0] || poweron_setting[1] != buf[1])
    {
        NVMCTRL_USER_ROW_RowErase(NVMCTRL_USERROW_START_ADDRESS);
        while (NVMCTRL_IsBusy())
            ;
        NVMCTRL_USER_ROW_PageWrite(buf, NVMCTRL_USERROW_START_ADDRESS);
        while (NVMCTRL_IsBusy())
            ;
    }
}

// *****************************************************************************
// *****************************************************************************
// 各種設定画面の表示
// *****************************************************************************
// *****************************************************************************

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
        set_low_frequency_clock();

        // キー読み取り開始
        TC2_TimerStart();
        // キー読み取りループ
        while (1)
        {
            WDT_Clear();
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
        // キー読み取りストップ
        TC2_TimerStop();

        set_high_frequency_clock();

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
// 科学定数
// *****************************************************************************
// *****************************************************************************

// 科学定数の定義
typedef struct
{
    const char *name;
    df_t (*get_value)(void);
    const char *symbol;
} scientific_constant_t;

// 科学定数を取得する関数群
df_t get_speed_of_light(void)
{
    df_t result;
    // 光の速度 2.99792458E8 m/s
    string_to_df("2.99792458E8", &result);
    return result;
}

df_t get_planck_constant(void)
{
    df_t result;
    // プランク定数 6.62607015E-34 J⋅s
    string_to_df("6.62607015E-34", &result);
    return result;
}

df_t get_elementary_charge(void)
{
    df_t result;
    // 電気素量 1.602176634E-19 C
    string_to_df("1.602176634-19", &result);
    return result;
}

df_t get_avogadro_number(void)
{
    df_t result;
    // アボガドロ数 6.02214076E23 /mol
    string_to_df("6.02214076E23", &result);
    return result;
}

df_t get_boltzmann_constant(void)
{
    df_t result;
    // ボルツマン定数 1.380649E-23 J/K
    string_to_df("1.380649E-23", &result);
    return result;
}

df_t get_gas_constant(void)
{
    df_t result;
    // 気体定数 8.314462618 J/(mol⋅K)
    string_to_df("8.314462618", &result);
    return result;
}

df_t get_electron_mass(void)
{
    df_t result;
    // 電子質量 9.1093837015E-31 kg
    string_to_df("9.1093837015E-31", &result);
    return result;
}

df_t get_proton_mass(void)
{
    df_t result;
    // 陽子質量 1.67262192369E-27 kg
    string_to_df("1.67262192369E-27", &result);
    return result;
}

df_t get_neutron_mass(void)
{
    df_t result;
    // 中性子質量 1.67492749804E-27 kg
    string_to_df("1.67492749804E-27", &result);
    return result;
}

df_t get_atomic_mass_unit(void)
{
    df_t result;
    // 原子質量定数 1.66053906660E-27 kg
    string_to_df("1.66053906660E-27", &result);
    return result;
}

df_t get_gravitational_constant(void)
{
    df_t result;
    // 万有引力定数 6.67430E-11 m³/(kg·s²)
    string_to_df("6.67430E-11", &result);
    return result;
}

df_t get_standard_gravity(void)
{
    df_t result;
    // 標準重力加速度 9.80665 m/s²
    string_to_df("9.80665", &result);
    return result;
}

df_t get_stefan_boltzmann_constant(void)
{
    df_t result;
    // ステファン-ボルツマン定数 5.670374419E-8 W/(m²·K⁴)
    string_to_df("5.670374419E-8", &result);
    return result;
}

df_t get_rydberg_constant(void)
{
    df_t result;
    // リュードベリ定数 1.0973731568160E7 m⁻¹
    string_to_df("1.0973731568160E7", &result);
    return result;
}

df_t get_vacuum_permeability(void)
{
    df_t result;
    // 真空透磁率 μ₀ 1.25663706212E-6 H/m
    string_to_df("1.25663706212E-6", &result);
    return result;
}

df_t get_vacuum_permittivity(void)
{
    df_t result;
    // 真空誘電率 ε₀ 8.8541878128E-12 F/m
    string_to_df("8.8541878128E-12", &result);
    return result;
}

df_t get_impedance_of_free_space(void)
{
    df_t result;
    // 自由空間インピーダンス √(μ₀/ε₀) 376.730313667 Ω
    string_to_df("376.730313667", &result);
    return result;
}

df_t get_fine_structure_constant(void)
{
    df_t result;
    // 微細構造定数 α 7.2973525693E-3
    string_to_df("7.2973525693E-3", &result);
    return result;
}

df_t get_bohr_radius(void)
{
    df_t result;
    // ボーア半径 5.29177210903E-11 m
    string_to_df("5.29177210903E-11", &result);
    return result;
}

df_t get_faraday_constant(void)
{
    df_t result;
    // ファラデー定数 96485.33212 C/mol
    string_to_df("9.648533212E4", &result);
    return result;
}

df_t get_molar_gas_constant(void)
{
    df_t result;
    // 気体定数 8.314462618 J/(mol·K)
    string_to_df("8.314462618", &result);
    return result;
}

df_t get_wien_displacement_constant(void)
{
    df_t result;
    // ウィーン変位則定数 2.897771955E-3 m·K
    string_to_df("2.897771955E-3", &result);
    return result;
}

// 科学定数の配列
static const scientific_constant_t constants[] = {
    // グループ1 (F1キー) - 基本物理定数
    {"Light Speed", get_speed_of_light, "c"},
    {"Planck h", get_planck_constant, "h"},
    {"Elem Charge", get_elementary_charge, "e"},
    {"Electron m", get_electron_mass, "me"},
    {"Proton m", get_proton_mass, "mp"},
    {"Neutron m", get_neutron_mass, "mn"},
    {"Boltzmann", get_boltzmann_constant, "k"},
    {"Avogadro", get_avogadro_number, "Na"},
    {"Gas Const", get_molar_gas_constant, "R"},
    {"Fine Struct", get_fine_structure_constant, "alpha"},

    // グループ2 (F2キー) - 応用物理定数・電磁気学定数
    {"Vacuum u", get_vacuum_permeability, "u0"},
    {"Vacuum e", get_vacuum_permittivity, "e0"},
    {"Free Space Z", get_impedance_of_free_space, "Z0"},
    {"Gravity G", get_gravitational_constant, "G"},
    {"Std Gravity", get_standard_gravity, "g"},
    {"Faraday", get_faraday_constant, "F"},
    {"Stefan-Boltz", get_stefan_boltzmann_constant, "s"},
    {"Wien Displ", get_wien_displacement_constant, "b"},
    {"Rydberg", get_rydberg_constant, "Rinf"},
    {"Bohr Radius", get_bohr_radius, "a0"}};

#define NUM_CONSTANTS (sizeof(constants) / sizeof(constants[0]))
#define CONSTANTS_PER_GROUP 10

// 科学定数選択メニュー
bool scientific_constants_menu(bool *push_flag, char *inputval, uint8_t group)
{
    AQM1602Y_Clear();
    uint8_t selected = 0;
    uint8_t key = 254;
    uint8_t last_key = 255;
    bool is_pushed = false;
    bool local_shift = false;

    // グループのオフセットを計算
    uint8_t group_offset = (group - 1) * CONSTANTS_PER_GROUP;

    // 初期表示
    AQM1602Y_Corsor_YX(0, 0);
    char display_line[17];
    snprintf(display_line, sizeof(display_line), "1.%s", constants[group_offset].symbol);
    AQM1602Y_Print(display_line, 16);
    AQM1602Y_Corsor_YX(1, 0);
    AQM1602Y_Print((char *)constants[group_offset].name, 16);

    while (1)
    {
        set_low_frequency_clock();

        // キー読み取り開始
        TC2_TimerStart();
        while (1)
        {
            WDT_Clear();
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
        TC0_Timer16bitCounterSet(0);
        TC2_TimerStop();

        set_high_frequency_clock();

        is_pushed = true;

        // キー解釈
        switch (key)
        {
        case KSHIFT:
            local_shift ^= true;
            break;
        case KENTER:
            // push_flag処理を実行
            if (*push_flag == true)
            {
                *push_flag = false;
                df_t tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            // 選択された定数を設定
            stack[0] = constants[group_offset + selected].get_value();
            return true;
        case KDEL:
            return false;
        case KF1:
            group = 1;
            group_offset = (group - 1) * CONSTANTS_PER_GROUP;
            break;
        case KF2:
            group = 2;
            group_offset = (group - 1) * CONSTANTS_PER_GROUP;
            break;
        case K0:
            if (9 < CONSTANTS_PER_GROUP)
            {
                selected = 9;
            }
            break;
        case K1:
            selected = 0;
            break;
        case K2:
            if (1 < CONSTANTS_PER_GROUP)
            {
                selected = 1;
            }
            break;
        case K3:
            if (2 < CONSTANTS_PER_GROUP)
            {
                selected = 2;
            }
            break;
        case K4:
            if (3 < CONSTANTS_PER_GROUP)
            {
                selected = 3;
            }
            break;
        case K5:
            if (4 < CONSTANTS_PER_GROUP)
            {
                selected = 4;
            }
            break;
        case K6:
            if (5 < CONSTANTS_PER_GROUP)
            {
                selected = 5;
            }
            break;
        case K7:
            if (6 < CONSTANTS_PER_GROUP)
            {
                selected = 6;
            }
            break;
        case K8:
            if (7 < CONSTANTS_PER_GROUP)
            {
                selected = 7;
            }
            break;
        case K9:
            if (8 < CONSTANTS_PER_GROUP)
            {
                selected = 8;
            }
            break;
        case KR: // 上下移動
            if (local_shift == false)
            {
                selected = (selected + 1) % CONSTANTS_PER_GROUP;
            }
            else
            {
                selected = (selected == 0) ? CONSTANTS_PER_GROUP - 1 : selected - 1;
            }
            break;
        case 0:
            is_pushed = false;
            break;
        default:
            break;
        }

        // 表示処理
        if (is_pushed)
        {
            // 上段：選択番号とシンボル
            AQM1602Y_Corsor_YX(0, 0);
            if (selected == 9)
            {
                snprintf(display_line, sizeof(display_line), "0.%s", constants[group_offset + selected].symbol);
            }
            else
            {
                snprintf(display_line, sizeof(display_line), "%d.%s", selected + 1, constants[group_offset + selected].symbol);
            }
            AQM1602Y_Print("                ", 16);
            AQM1602Y_Corsor_YX(0, 0);
            AQM1602Y_Print(display_line, 16);

            // 矢印表示
            if (local_shift == true)
            {
                AQM1602Y_Corsor_YX(0, 15);
                AQM1602Y_Print("\x01", 1); // 上向き矢印
            }
            else
            {
                AQM1602Y_Corsor_YX(0, 15);
                AQM1602Y_Print("\x02", 1); // 下向き矢印
            }

            // 下段：定数名
            AQM1602Y_Corsor_YX(1, 0);
            AQM1602Y_Print("                ", 16);
            AQM1602Y_Corsor_YX(1, 0);
            AQM1602Y_Print((char *)constants[group_offset + selected].name, 16);
        }

        delay_ms(5);
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
    // 割り込み周期 600,000msec (10min)
    TC0_TimerCallbackRegister(pdown, (uintptr_t)NULL);
    TC0_TimerStart();

    // WDT
    WDT_TimeoutPeriodSet(0x9); // 4秒
    WDT_Enable();

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
    char inputval[MAX_INPUTVAL_LENGTH] = {0};
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
    int_to_df(0, &last_x);
    // モード設定表示フラグ
    bool mode_flag = false;
    // 表示設定表示フラグ
    bool disp_flag = false;
    while (1)
    {
        set_low_frequency_clock();

        // キー読み取り開始
        TC2_TimerStart();

        // キー読み取りループ
        while (1)
        {
            WDT_Clear();
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
        // キー読み取りストップ
        TC2_TimerStop();

        set_high_frequency_clock();

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
                if (0 < inputval_len && inputval_len < MAX_INPUTVAL_LENGTH)
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
                    if (inputval_len == 0)
                    {
                        pri_m = false;
                    }
                }
                else
                {
                    for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                    {
                        inputval[i] = '\0';
                    }
                }
            }
            string_to_df(inputval, &stack[0]);
            shift = false;
            break;
        case K0:
            handle_digit_input('0', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K1:
            handle_digit_input('1', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K2:
            handle_digit_input('2', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K3:
            handle_digit_input('3', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K4:
            handle_digit_input('4', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K5:
            handle_digit_input('5', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K6:
            handle_digit_input('6', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K7:
            handle_digit_input('7', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K8:
            handle_digit_input('8', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case K9:
            handle_digit_input('9', inputval, &inputval_len, &push_flag, stack,
                               &pos_E, &pos_dot, &pri_m, &pri_e);
            shift = false;
            break;
        case KDOT:
            if (push_flag == true)
            {
                push_flag = false;
                tmp = stack[0];
                stack_push();
                for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (shift == true)
            {
                stack[0] = df_e();
                after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
                break;
            }
            else
            {
                if (pos_dot == 0 && pos_E == 0 && inputval_len != 0)
                {
                    if (inputval_len < MAX_INPUTVAL_LENGTH)
                    {
                        inputval[inputval_len] = '.';
                        pos_dot = inputval_len;
                        inputval_len++;
                    }
                }
                else if (pos_dot == 0 && pos_E == 0 && inputval_len == 0)
                {
                    if (inputval_len < MAX_INPUTVAL_LENGTH)
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
                for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (shift == true)
            {
                stack[0] = df_pi();
                after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
                break;
            }
            else
            {
                if (pos_E == 0 && inputval_len != 0)
                {
                    if (inputval_len < MAX_INPUTVAL_LENGTH)
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
                for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                {
                    inputval[i] = '\0';
                }
                stack[0] = tmp;
            }
            if (pos_E == 0)
            {
                if (pri_m == false) // 負にするとき
                {
                    pri_m = true;
                    inputval_len++;
                    pos_dot = (pos_dot == 0) ? 0 : pos_dot + 1;
                    for (int8_t i = MAX_INPUTVAL_LENGTH - 2; i >= 0; i--)
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
                    for (int8_t i = 0; i < MAX_INPUTVAL_LENGTH - 2; i++)
                    {
                        inputval[i] = inputval[i + 1];
                    }
                    inputval[inputval_len] = '\0';
                }
            }
            else // 指数部の符号
            {
                if (pri_e == false) // 負にするとき
                {
                    pri_e = true;
                    inputval_len++;
                    for (int8_t i = MAX_INPUTVAL_LENGTH - 2; i > pos_E; i--)
                    {
                        inputval[i + 1] = inputval[i];
                    }
                    inputval[pos_E + 1] = '-';
                }
                else
                {
                    pri_e = false;
                    inputval_len--;
                    for (int8_t i = pos_E + 1; i < MAX_INPUTVAL_LENGTH - 2; i++)
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

            for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
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
                if (push_flag == true)
                {
                    stack_push();
                }
                for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
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
            after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
            break;
        case KSUB:
            last_x = stack[0];
            df_sub(&stack[1], &stack[0], &tmp);
            stack_pop();
            stack[0] = tmp;
            after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
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
            }
            else
            {
                // 科学定数メニューを表示 (グループ1)
                bool constant_selected = scientific_constants_menu(&push_flag, inputval, 1);
                if (constant_selected)
                {
                    after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
                }
                // メニュー終了後、キー状態を完全にリセット
                clear_key();
            }
            break;
        case KF2:
            if (shift == true)
            {
                disp_flag = true;
                mode_flag = false;
                shift = false;
            }
            else
            {
                // 科学定数メニューを表示 (グループ2)
                bool constant_selected = scientific_constants_menu(&push_flag, inputval, 2);
                if (constant_selected)
                {
                    after_operation(inputval, &inputval_len, &pos_E, &pos_dot, &pri_m, &pri_e, &shift, &push_flag);
                }
                // メニュー終了後、キー状態を完全にリセット
                clear_key();
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
            } // Xレジスタの表示
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
                    for (uint8_t i = 0; i < MAX_INPUTVAL_LENGTH; i++)
                    {
                        inputval[i] = '\0';
                    }
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
