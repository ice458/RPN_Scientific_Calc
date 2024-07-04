#ifndef DECIMAL_FLOAT_H
#define DECIMAL_FLOAT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// 内部計算桁数
#define DECIMAL_FLOAT_MANTISSA_SIZE 18
// 表示桁数
#define DECIMAL_FLOAT_DISPLAY_DIGIT 10

// 10進浮動小数点数
typedef struct
{
    // 例
    // [0][1,0,0][0]→1.0E0
    // [0][3,1,4][-1]→3.14E-1
    // [1][3,1,4][2]→-3.14E2
    uint8_t sign;                                  // 符号
    uint8_t mantissa[DECIMAL_FLOAT_MANTISSA_SIZE]; // 仮数部
    int16_t exponent;                              // 指数部
} df_t;

// 表示モード
typedef enum
{
    DF_STRING_MODE_ENGINEERING, // 指数表示
    DF_STRING_MODE_SCIENTIFIC   // 科学表示
} df_string_mode_t;

// 角度モード
typedef enum
{
    DF_ANGLE_MODE_DEG, // 度数法
    DF_ANGLE_MODE_RAD  // 弧度法
} df_angle_mode_t;

#define DF_0  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0}
#define DF_05  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5}, -1}
#define DF_1  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0}
#define DF_2  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 0}
#define DF_3  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3}, 0}
#define DF_4  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4}, 0}
#define DF_5  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5}, 0}
#define DF_10  {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 1}
#define DF_m1 {1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0}
#define DF_m2 {1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 0}

#define DF_PI   {0, {4, 2, 3, 9, 7, 9, 8, 5, 3, 5, 6, 2, 9, 5, 1, 4, 1, 3}, 0}
#define DF_2PI  {0, {8, 4, 6, 8, 5, 9, 7, 1, 7, 0, 3, 5, 8, 1, 3, 8, 2, 6}, 0}
#define DF_PId2 {0, {2, 6, 6, 9, 8, 4, 9, 7, 6, 2, 3, 6, 9, 7, 0, 7, 5, 1}, 0}

#define DF_E {0, {4, 2, 5, 4, 0, 9, 5, 4, 8, 2, 8, 1, 8, 2, 8, 1, 7, 2}, 0}

// 整数の累乗
int16_t pow_int(int16_t x, int16_t n);
// 交換
void df_swap(df_t *a, df_t *b);
// 浮動小数点数から10進浮動小数点数への変換
void double_to_df(double f, df_t *a);
// 整数から10進浮動小数点数への変換
void int_to_df(int16_t value, df_t *df);
// 10進浮動小数点数から浮動小数点数への変換
void df_to_double(df_t *a, double *f);
// 整数に丸める
void df_round_int(df_t *a, df_t *result);
// ゼロ判定
bool df_is_zero(df_t *a);
// 仮数部の比較
int8_t df_compare_mantissa(df_t *a, df_t *b);
// 比較
int8_t df_compare(df_t *a, df_t *b);
// a+b
void df_add(df_t *a, df_t *b, df_t *result);
// a-b
void df_sub(df_t *a, df_t *b, df_t *result);
// a*b
void df_mul(df_t *a, df_t *b, df_t *result);
// a^n (nは整数)
void df_pow_int(df_t *a, int16_t n, df_t *result);
// a!
void df_factorial(df_t *a, df_t *result);
// 1/a
void df_inv(df_t *a, df_t *result);
// a/b
void df_div(df_t *a, df_t *b, df_t *result);
// a%b
void df_mod(df_t *a, df_t *b, df_t *result);
// sqrt(a)
void df_sqrt(df_t *a, df_t *result);
// cube_root(a)
void df_cbrt(df_t *a, df_t *result);
// nth_root(a, n)
void df_nth_root(df_t *a, df_t *n, df_t *result);
// exp(a)
void df_exp(df_t *x, df_t *result);
// ln(x)
void df_ln(df_t *x, df_t *result);
// log10(x)
void df_log10(df_t *x, df_t *result);
// logx(y)
void df_log(df_t *x, df_t *y, df_t *result);
// a^b
void df_pow(df_t *a, df_t *b, df_t *result);
// pow10(x)
void df_pow10(df_t *x, df_t *result);
// 角度モードを設定
void set_df_angle_mode(df_angle_mode_t mode);
// 角度モードを取得
df_angle_mode_t get_df_angle_mode();
// sin(a)
void df_sin(df_t *a, df_t *result);
// cos(a)
void df_cos(df_t *a, df_t *result);
// tan(a)
void df_tan(df_t *a, df_t *result);
// arcsin(a)
void df_asin(df_t *a, df_t *result);
// arccos(a)
void df_acos(df_t *a, df_t *result);
// arctan(a)
void df_atan(df_t *a, df_t *result);
// pi
df_t df_pi();
// deg to rad
void df_deg_to_rad(df_t *deg, df_t *result);
// rad to deg
void df_rad_to_deg(df_t *rad, df_t *result);
// e
df_t df_e();
// (a*b)/(a+b)
void df_mul_over_sum(df_t *a, df_t *b, df_t *result);
// fc(r,c)
void df_fc_rc(df_t *r, df_t *c, df_t *result);
// fc(l,c)
void df_fc_lc(df_t *l, df_t *c, df_t *result);
// 表示桁数の一桁下で四捨五入
void df_round(df_t *a, df_t *result);
// 10進浮動小数点数の表示モードを設定
void set_df_string_mode(df_string_mode_t mode);
// 10進浮動小数点数の表示モードを取得
df_string_mode_t get_df_string_mode();
// 10進浮動小数点数を文字列に変換
void df_to_string(df_t *a, char *str);
// 数値文字列を正規化
void normalize_df_string(char *input, char *output);
// 文字列を10進浮動小数点数に変換
void string_to_df(char *in, df_t *a);

#endif // DECIMAL_FLOAT_H