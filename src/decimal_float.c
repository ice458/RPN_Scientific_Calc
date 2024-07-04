#include "decimal_float.h"

// 整数の累乗
int16_t pow_int(int16_t x, int16_t n)
{
    int16_t result = 1;
    for (int16_t i = 0; i < n; i++)
    {
        result *= x;
    }
    return result;
}

// エラーコード生成
void df_error(df_t *a)
{
    a->sign = 1;
    a->exponent = 0;
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        a->mantissa[i] = 0;
    }
}

// 交換
void df_swap(df_t *a, df_t *b)
{
    df_t tmp = *a;
    *a = *b;
    *b = tmp;
}

// 浮動小数点数から10進浮動小数点数への変換
void double_to_df(double f, df_t *a)
{
    a->sign = f < 0 ? 1 : 0;
    f = f < 0 ? -f : f;
    a->exponent = 0;

    while (f >= 10)
    {
        f /= 10;
        a->exponent++;
    }
    while (0 < f && f < 1)
    {
        f *= 10;
        a->exponent--;
    }

    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= 0; i--)
    {
        a->mantissa[i] = (uint8_t)f;
        f = (f - a->mantissa[i]) * 10;
    }
}

// 整数から10進浮動小数点数への変換
void int_to_df(int16_t value, df_t *df)
{
    if (value < 0)
    {
        df->sign = 1;
        value = -value;
    }
    else if (value == 0)
    {
        df->sign = 0;
        df->exponent = 0;
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            df->mantissa[i] = 0;
        }
        return;
    }
    else
    {
        df->sign = 0;
    }

    int16_t temp_value = value;
    int8_t digit_count = 0;

    while (temp_value > 0)
    {
        digit_count++;
        temp_value /= 10;
    }

    df->exponent = digit_count - 1;

    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - digit_count + 1; i++)
    {
        df->mantissa[i] = 0;
    }
    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - digit_count; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        df->mantissa[i] = value % 10;
        value /= 10;
    }
}

// 10進浮動小数点数から浮動小数点数への変換
void df_to_double(df_t *a, double *f)
{
    *f = 0;
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        *f += a->mantissa[i];
        if (i < DECIMAL_FLOAT_MANTISSA_SIZE - 1)
        {
            *f /= 10;
        }
    }
    if (a->exponent > 0)
    {
        for (int16_t i = 0; i < a->exponent; i++)
        {
            *f *= 10;
        }
    }
    else
    {
        for (int16_t i = 0; i < -1 * a->exponent; i++)
        {
            *f /= 10;
        }
    }

    if (a->sign)
    {
        *f = -*f;
    }
}

// 整数に丸める
void df_round_int(df_t *a, df_t *result)
{
    df_t tmp = *a;
    int8_t digit = 0;
    result->sign = tmp.sign;
    result->exponent = tmp.exponent;
    digit = DECIMAL_FLOAT_MANTISSA_SIZE - 1 - tmp.exponent;
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        result->mantissa[i] = 0;
    }
    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= digit; i--)
    {
        result->mantissa[i] = tmp.mantissa[i];
    }
    if (df_is_zero(result))
    {
        int_to_df(0, result);
    }
}

// ゼロ判定
bool df_is_zero(df_t *a)
{
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        if (a->mantissa[i] != 0)
        {
            return false;
        }
    }
    return true;
}

// 仮数部の比較
int8_t df_compare_mantissa(df_t *a, df_t *b)
{
    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= 0; i--)
    {
        if (a->mantissa[i] > b->mantissa[i])
        {
            if (a->sign == 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
        if (a->mantissa[i] < b->mantissa[i])
        {
            if (a->sign == 0)
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
    return 0;
}

// 比較
int8_t df_compare(df_t *a, df_t *b)
{
    if (a->sign > b->sign)
    {
        return -1;
    }
    if (a->sign < b->sign)
    {
        return 1;
    }
    if (df_is_zero(a) == false && df_is_zero(b) == false)
    {

        if (a->exponent > b->exponent)
        {
            if (a->sign == 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
        if (a->exponent < b->exponent)
        {
            if (a->sign == 0)
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
    return df_compare_mantissa(a, b);
}

// BCD加算(一桁)
uint8_t bcd_add(uint8_t a, uint8_t b, uint8_t *c)
{
    int8_t sum = a + b + *c;
    if (sum >= 10)
    {
        sum += 6;
        *c = 1;
    }
    else
    {
        *c = 0;
    }
    return sum & 0x0F;
}

// BCD減算(一桁)
uint8_t bcd_sub(uint8_t a, uint8_t b, uint8_t *c)
{
    int8_t diff = a - b - *c;
    if (diff < 0)
    {
        diff += 10;
        *c = 1;
    }
    else
    {
        *c = 0;
    }
    return diff & 0x0F;
}

// BCD乗算(一桁)
uint8_t bcd_mul(uint8_t a, uint8_t b, uint8_t *c)
{
    int8_t mul = a * b + *c;
    *c = mul / 10;
    return mul % 10;
}

// a+b
void df_add(df_t *a, df_t *b, df_t *result)
{
    df_t larger = *a;
    df_t smaller = *b;

    if (df_compare(&larger, &smaller) < 0)
    {
        larger = *b;
        smaller = *a;
    }
    // どちらかがゼロの場合
    if (df_is_zero(&larger))
    {
        *result = smaller;
        return;
    }
    else if (df_is_zero(&smaller))
    {
        *result = larger;
        return;
    }

    int16_t shift = larger.exponent - smaller.exponent;
    uint8_t carry = 0;
    uint8_t carry2 = 0;
    bool round_add = false;
    bool round_sub = false;

    // シフト量が正の場合
    if (shift >= 0)
    {
        // 丸め処理用のフラグ
        if (shift <= DECIMAL_FLOAT_MANTISSA_SIZE) // バッファオーバーフローを防ぐ
        {
            if (smaller.mantissa[shift - 1] >= 5)
            {
                round_add = true;
            }
            if (smaller.mantissa[shift - 1] > 5)
            {
                round_sub = true;
            }
        } // シフト量が仮数部のサイズを超える場合
        else
        {
            shift = DECIMAL_FLOAT_MANTISSA_SIZE;
        }
        // 仮数部をシフトして桁をそろえる
        for (int16_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - shift; i++)
        {
            smaller.mantissa[i] = smaller.mantissa[i + shift];
        }
        // 0埋め
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - shift; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            smaller.mantissa[i] = 0;
        }
        result->exponent = larger.exponent;
    } // シフト量が負の場合。
      // これは、大きな数の指数部が小さな数の指数部より小さいときに発生する。
      // 例：2E-3 - 1E0
      // 例：-0.1 - 0.005
    else
    {
        shift = -shift;
        // 丸め処理用のフラグ
        if (shift <= DECIMAL_FLOAT_MANTISSA_SIZE) // バッファオーバーフローを防ぐ
        {
            if (larger.mantissa[shift - 1] >= 5)
            {
                round_add = true;
            }
            if (larger.mantissa[shift - 1] > 5)
            {
                round_sub = true;
            }
        } // シフト量が仮数部のサイズを超える場合
        else
        {
            shift = DECIMAL_FLOAT_MANTISSA_SIZE;
        }
        // 仮数部をシフトして桁をそろえる
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - shift; i++)
        {
            larger.mantissa[i] = larger.mantissa[i + shift];
        }
        // 0埋め
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - shift; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            larger.mantissa[i] = 0;
        }
        result->exponent = smaller.exponent;
    }

    // 絶対値の足し算
    if (a->sign == b->sign)
    {
        // 丸め処理
        if (round_add == true)
        {
            carry = 1;
        }
        // 仮数部の加算
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            result->mantissa[i] = bcd_add(larger.mantissa[i], smaller.mantissa[i], &carry);
        }
        // 桁上がり処理、指数部の決定
        if (carry)
        {
            if (result->mantissa[0] > 5)
            {
                carry2 = 1;
            }
            for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - 1; i++)
            {
                result->mantissa[i] = result->mantissa[i + 1];
            }
            result->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] = 1;
            result->exponent++;

            // 桁上がりに伴う有効桁数の増加を丸める
            if (carry2 == 1)
            {
                carry = 0;
                result->mantissa[0] = bcd_add(result->mantissa[0], smaller.mantissa[0], &carry);
                for (int8_t i = 1; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
                {
                    result->mantissa[i] = bcd_add(result->mantissa[i], smaller.mantissa[i], &carry);
                    if (carry == 0)
                    {
                        break;
                    }
                }
            }
        }
        // 符号
        result->sign = larger.sign;
    } // 引き算
    else
    {
        // 丸め処理
        if (round_sub == true)
        {
            carry = 1;
        }

        int8_t greater = 0;
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= 0; i--)
        {
            if (larger.mantissa[i] > smaller.mantissa[i])
            {
                greater = 1;
                break;
            }
            if (larger.mantissa[i] < smaller.mantissa[i])
            {
                greater = -1;
                break;
            }
        }

        if (greater == 0)
        {
            int_to_df(0, result);
            return;
        }
        else if (greater == 1)
        {
            result->sign = 0;
        }
        else
        {
            result->sign = 1;
        }

        if (greater != -1)
        {
            // 仮数部の減算
            for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
            {
                result->mantissa[i] = bcd_sub(larger.mantissa[i], smaller.mantissa[i], &carry);
            }
        }
        else
        {
            // 仮数部の減算
            for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
            {
                result->mantissa[i] = bcd_sub(smaller.mantissa[i], larger.mantissa[i], &carry);
            }
        }

        // ゼロではない桁が最上位に来るようにシフト
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= 0; i--)
        {
            if (result->mantissa[i] != 0)
            {
                for (int8_t j = 0; j < DECIMAL_FLOAT_MANTISSA_SIZE - 1 - i; j++)
                {
                    for (int8_t k = DECIMAL_FLOAT_MANTISSA_SIZE - 1; k >= 1; k--)
                    {
                        result->mantissa[k] = result->mantissa[k - 1];
                    }
                    result->mantissa[0] = 0;
                    larger.exponent--;
                    smaller.exponent--;
                }
                break;
            }
            else
            {
                if (i == 0 && result->mantissa[i] == 0)
                {
                    larger.exponent = 0;
                    smaller.exponent = 0;
                }
            }
        }

        // 指数部の決定
        if (greater == 1)
        {
            result->exponent = larger.exponent;
        }
        else
        {
            result->exponent = smaller.exponent;
        }
    }
    if (df_is_zero(result))
    {
        result->sign = 0;
    }
}

// a-b
void df_sub(df_t *a, df_t *b, df_t *result)
{
    df_t tmp = *b;
    tmp.sign = !b->sign;
    df_add(a, &tmp, result);
}

// a*b
void df_mul(df_t *a, df_t *b, df_t *result)
{
    uint8_t carry[DECIMAL_FLOAT_MANTISSA_SIZE] = {0};
    uint16_t temp[DECIMAL_FLOAT_MANTISSA_SIZE * 2] = {0};
    uint8_t top_digit_pos = 0;
    // 仮数部の乗算
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
    {
        for (int8_t j = 0; j < DECIMAL_FLOAT_MANTISSA_SIZE; j++)
        {
            temp[i + j] += bcd_mul(a->mantissa[i], b->mantissa[j], &carry[j]);
            temp[i + j + 1] += carry[j];
            carry[j] = 0;
        }
    }
    // 桁上がり処理
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE * 2 - 1; i++)
    {
        if (temp[i] >= 10)
        {
            temp[i + 1] = temp[i + 1] + (temp[i] / 10);
            temp[i] %= 10;
        }
    }
    // 最上位桁の位置を取得
    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE * 2 - 1; i >= 0; i--)
    {
        if (temp[i] != 0)
        {
            top_digit_pos = i;
            break;
        }
    }
    // 全ての桁がゼロの場合
    if (top_digit_pos == 0 && temp[0] == 0)
    {
        result->sign = 0;
        result->exponent = 0;
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            result->mantissa[i] = 0;
        }
        return;
    }
    // 仮数部の決定
    for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= 0; i--)
    {
        result->mantissa[i] = temp[i + top_digit_pos - DECIMAL_FLOAT_MANTISSA_SIZE + 1];
    }
    // 丸め処理
    if (top_digit_pos > DECIMAL_FLOAT_MANTISSA_SIZE)
    {
        if (temp[top_digit_pos - DECIMAL_FLOAT_MANTISSA_SIZE] >= 5)
        {
            result->mantissa[0]++;
            // 桁上がり処理
            for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - 1; i++)
            {
                if (result->mantissa[i] >= 10)
                {
                    result->mantissa[i + 1] = result->mantissa[i + 1] + (result->mantissa[i] / 10);
                    result->mantissa[i] %= 10;
                }
            }
        }
    }
    // 指数部の決定
    int8_t shift = DECIMAL_FLOAT_MANTISSA_SIZE * 2 - top_digit_pos;
    result->exponent = a->exponent + b->exponent - shift + 2;

    // 符号の決定
    result->sign = a->sign ^ b->sign;
}

// a^n (nは整数)
void df_pow_int(df_t *a, int16_t n, df_t *result)
{
    df_t tmp = DF_1;
    for (int16_t i = 0; i < n; i++)
    {
        df_mul(&tmp, a, &tmp);
    }
    *result = tmp;
}

// a!
void df_factorial(df_t *a, df_t *result)
{
    df_t i_df = DF_1;
    df_t tmp = DF_1;
    df_t one = DF_1;

    for (; df_compare(&i_df, a) <= 0; df_add(&i_df, &one, &i_df))
    {
        df_mul(&tmp, &i_df, &tmp);
    }
    *result = tmp;
}

// 1/a
void df_inv(df_t *a, df_t *result)
{
    if (df_is_zero(a))
    {
        df_error(result);
        return;
    }
    df_t n, tmp;
    double a_f;
    df_to_double(a, &a_f);
    df_t one = DF_1;
    df_t two = DF_2;

    // 初期値の設定
    double_to_df(1.0 / a_f, &n);
    // ニュートン法で1/aを求める
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE + 1; i++)
    {
        df_mul(a, &n, &tmp);
        df_sub(&two, &tmp, &tmp);
        df_mul(&n, &tmp, &n);
    }

    // 丸め処理
    // これでいいのかわからん。無限小数の時、最終桁を必ず1大きくしてしまう。
    // 表示する桁より多く計算しているならこれでもいいのでは。
    df_mul(&n, a, &tmp);
    if (df_compare(&one, &tmp) == 1)
    {
        tmp = n;
        tmp.mantissa[0] = 1;
        for (int8_t i = 1; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            tmp.mantissa[i] = 0;
        }
        df_add(&tmp, &n, &n);
    }
    *result = n;
}

// a/b
void df_div(df_t *a, df_t *b, df_t *result)
{
    df_t tmp;
    if (df_is_zero(b))
    {
        df_error(result);
        return;
    }
    df_inv(b, &tmp);
    df_mul(a, &tmp, result);
}

// a%b
void df_mod(df_t *a, df_t *b, df_t *result)
{
    df_t tmp;
    df_div(a, b, &tmp);
    df_round_int(&tmp, &tmp);
    df_mul(b, &tmp, &tmp);
    df_sub(a, &tmp, result);
}

// sqrt(a)
void df_sqrt(df_t *a, df_t *result)
{
    if (a->sign)
    {
        df_error(result);
        return;
    }
    df_t n, tmp, tree, point_five;
    double a_f;
    df_to_double(a, &a_f);
    double_to_df(0.5, &point_five);
    int_to_df(3, &tree);

    // 初期値の設定
    double_to_df(1.0 / sqrt(a_f), &n);
    // ニュートン法で1/sqrt(a)を求める
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE + 1; i++)
    {
        df_mul(&n, &n, &tmp);
        df_mul(a, &tmp, &tmp);
        df_sub(&tree, &tmp, &tmp);
        df_mul(&point_five, &tmp, &tmp);
        df_mul(&n, &tmp, &n);
    }
    df_mul(&n, a, &n);

    // 丸め処理
    // これで良いのかわからん。無限小数の時、最終桁を必ず1大きくしてしまう。
    df_mul(&n, &n, &tmp);
    if (df_compare(a, &tmp) == 1)
    {
        tmp = n;
        tmp.mantissa[0] = 1;
        for (int8_t i = 1; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            tmp.mantissa[i] = 0;
        }
        df_add(&tmp, &n, &n);
    }
    *result = n;
}

// cube_root(a)
void df_cbrt(df_t *a, df_t *result)
{
    if (a->sign)
    {
        df_error(result);
        return;
    }
    double x_f;
    df_t y;
    df_to_double(a, &x_f);
    double_to_df(pow(x_f, 1.0 / 3.0), &y);
    *result = y;
}

// nth_root(a, n)
void df_nth_root(df_t *a, df_t *n, df_t *result)
{
    if (a->sign || df_is_zero(n))
    {
        df_error(result);
        return;
    }
    double a_f, n_f;
    df_t y;
    df_to_double(a, &a_f);
    df_to_double(n, &n_f);
    double_to_df(pow(a_f, 1.0 / n_f), &y);
    *result = y;
}

// exp(x)
// void df_exp(df_t *x, df_t *result)
// {
//     df_t y;
//     df_t e = df_e();
//     df_pow(&e, x, &y);
//     *result = y;
// }
void df_exp(df_t *x, df_t *result)
{
    const uint8_t loop = 30;
    df_t tmp, xtmp, sum, e2, ex2;
    bool inv_flag = false;
    df_t one = DF_1;
    df_t two = DF_2;
    df_t th = DF_3;

    tmp = one;
    xtmp = *x;
    sum = one;
    e2 = df_e();
    df_mul(&e2, &e2, &e2); // e^2
    ex2 = one;

    // 1/i
    df_t c0[31] = {
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5}, -1},
        {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, -1},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 2}, -1},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, -1},
        {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1}, -1},
        {0, {7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1}, -1},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 2, 1}, -1},
        {0, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, -1},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, -1},
        {0, {1, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9}, -2},
        {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8}, -2},
        {0, {1, 3, 2, 9, 6, 7, 0, 3, 2, 9, 6, 7, 0, 3, 2, 9, 6, 7}, -2},
        {0, {6, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7}, -2},
        {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6}, -2},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 2, 6}, -2},
        {0, {9, 5, 0, 7, 4, 6, 7, 1, 1, 4, 9, 2, 5, 3, 2, 8, 8, 5}, -2},
        {0, {6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5}, -2},
        {0, {1, 1, 2, 4, 8, 6, 3, 7, 4, 9, 8, 7, 5, 1, 3, 6, 2, 5}, -2},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5}, -2},
        {0, {0, 9, 1, 6, 7, 4, 0, 9, 1, 6, 7, 4, 0, 9, 1, 6, 7, 4}, -2},
        {0, {5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4}, -2},
        {0, {4, 7, 1, 2, 5, 6, 5, 9, 6, 8, 0, 6, 2, 8, 7, 4, 3, 4}, -2},
        {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 4}, -2},
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4}, -2},
        {0, {5, 1, 6, 4, 8, 3, 5, 1, 6, 4, 8, 3, 5, 1, 6, 4, 8, 3}, -2},
        {0, {0, 7, 3, 0, 7, 3, 0, 7, 3, 0, 7, 3, 0, 7, 3, 0, 7, 3}, -2},
        {0, {3, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 3}, -2},
        {0, {2, 5, 5, 6, 9, 8, 6, 0, 2, 6, 8, 5, 7, 2, 8, 4, 4, 3}, -2},
        {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, -2},
    };

    if (xtmp.sign == 1)
    {
        xtmp.sign = 0;
        inv_flag = true;
    }

    // 収束速度が落ちるので,e^x=e^2*e^(x-2)を利用して計算する
    while (df_compare(&xtmp, &th) >= 0)
    {
        df_sub(&xtmp, &two, &xtmp); // x-2
        df_mul(&e2, &ex2, &ex2);
    }
    // calc e^(x-2)
    tmp = one;
    for (int8_t i = 1; i < loop; i++)
    {
        df_mul(&tmp, &c0[i], &tmp);
        df_mul(&xtmp, &tmp, &tmp);
        df_add(&sum, &tmp, &sum);
    }
    df_mul(&ex2, &sum, &sum);
    if (inv_flag)
    {
        df_inv(&sum, &sum);
    }
    *result = sum;
}

// ln(x)
void df_ln(df_t *x, df_t *result)
{
    double x_f;
    df_t y;
    df_to_double(x, &x_f);
    double_to_df(log(x_f), &y);
    *result = y;
}
// void df_ln(df_t *x, df_t *result)
// {
//     if (df_is_zero(x))
//     {
//         result->sign = 0;
//         result->exponent = 0;
//         for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
//         {
//             result->mantissa[i] = 0;
//         }
//         return;
//     }
//     if (x->sign)
//     {
//         result->sign = 1;
//         result->exponent = 0;
//         for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
//         {
//             result->mantissa[i] = 0;
//         }
//         return;
//     }

//     const uint8_t loop = 30;
//     df_t xtmp, i_df;
//     xtmp = *x;
//     bool sub_flag = false;
//     df_t zero = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};
//     df_t one = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0};
//     df_t two = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 0};
//     df_t th = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5}, -1};
//     df_t tmp = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3}, 0};
//     df_t a = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0};
//     df_t lnth = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0};
//     df_t sum = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};
//     df_t sumlnth = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};

//     // 2以上で収束しないのでlnx=ln1/(1/x)=ln1-ln1/xを利用して計算する
//     if (df_compare(&xtmp, &two) >= 0)
//     {
//         sub_flag = true;
//         df_inv(&xtmp, &xtmp);
//     }

//     // 小さい値で収束が悪いので、ln(x)=ln(th*x/th)=ln(th)+ln(x/th)を利用して計算する
//     if (df_compare(&xtmp, &th) < 0)
//     {
//         // calc ln(th)
//         for (uint8_t i = 1; i < loop; i++)
//         {
//             int_to_df(i, &i_df);
//             df_sub(&th, &one, &tmp);
//             df_mul(&a, &tmp, &a);
//             df_div(&a, &i_df, &tmp);
//             if (i % 2 == 0)
//             {
//                 df_sub(&sum, &tmp, &sum);
//             }
//             else
//             {
//                 df_add(&sum, &tmp, &sum);
//             }
//         }
//         lnth = sum;
//         tmp = one;
//         a = one;
//         sum = zero;
//         while (df_compare(&xtmp, &th) < 0)
//         {
//             df_add(&lnth, &sumlnth, &sumlnth);
//             df_div(&xtmp, &th, &xtmp);
//         }
//         // calc ln(x/(th)^n)
//         for (uint8_t i = 1; i < loop; i++)
//         {
//             int_to_df(i, &i_df);
//             df_sub(&xtmp, &one, &tmp);
//             df_mul(&a, &tmp, &a);
//             df_div(&a, &i_df, &tmp);
//             if (i % 2 == 0)
//             {
//                 df_sub(&sum, &tmp, &sum);
//             }
//             else
//             {
//                 df_add(&sum, &tmp, &sum);
//             }
//         }
//         df_add(&sumlnth, &sum, &sum);
//         if (sub_flag)
//         {
//             sum.sign ^= sum.sign;
//         }
//         *result = sum;
//         return;
//     }

//     for (uint8_t i = 1; i < loop; i++)
//     {
//         int_to_df(i, &i_df);
//         df_sub(&xtmp, &one, &tmp);
//         df_mul(&a, &tmp, &a);
//         df_div(&a, &i_df, &tmp);
//         if (i % 2 == 0)
//         {
//             df_sub(&sum, &tmp, &sum);
//         }
//         else
//         {
//             df_add(&sum, &tmp, &sum);
//         }
//     }
//     if (sub_flag)
//     {
//         sum.sign ^= sum.sign;
//     }
//     *result = sum;
// }

// log10(x)
void df_log10(df_t *x, df_t *result)
{
    double x_f;
    df_t y;
    df_to_double(x, &x_f);
    double_to_df(log10(x_f), &y);
    *result = y;
}
// void df_log10(df_t *x, df_t *result)
// {
//     df_t tmp, ln10;
//     if (x->sign)
//     {
//         result->sign = 1;
//         result->exponent = 0;
//         for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
//         {
//             result->mantissa[i] = 0;
//         }
//         return;
//     }
//     df_t ten={0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 1};
//     df_ln(&ten, &ln10);
//     df_ln(x, &tmp);
//     df_div(&tmp, &ln10, result);
// }

// logx(y)
void df_log(df_t *x, df_t *y, df_t *result)
{
    if (x->sign || y->sign)
    {
        df_error(result);
        return;
    }
    df_t lnx, lny;
    df_ln(x, &lnx);
    df_ln(y, &lny);
    df_div(&lny, &lnx, result);
}

// a^b
void df_pow(df_t *a, df_t *b, df_t *result)
{
    double a_f, b_f;
    df_t y;
    df_to_double(a, &a_f);
    df_to_double(b, &b_f);
    double_to_df(pow(a_f, b_f), &y);
    *result = y;
}

// pow10(x)
void df_pow10(df_t *x, df_t *result)
{
    double x_f;
    df_t y;
    df_to_double(x, &x_f);
    double_to_df(pow(10.0, x_f), &y);
    *result = y;
}

// 角度モード
df_angle_mode_t angle_mode = DF_ANGLE_MODE_RAD;
void set_df_angle_mode(df_angle_mode_t mode)
{
    angle_mode = mode;
}

df_angle_mode_t get_df_angle_mode()
{
    return angle_mode;
}

// sin(a)
void df_sin(df_t *a, df_t *result)
{
    const uint8_t loop = 10;
    df_t a_tmp, tmp, tmp2, pi;
    a_tmp = *a;
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_deg_to_rad(&a_tmp, &a_tmp);
    }

    df_t one = DF_1;
    df_t mone = DF_m1;

    pi = df_pi();
    df_t m_pi = pi;
    m_pi.sign = 1;
    df_t pi2 = DF_2PI;
    df_t pi_2 = DF_PId2;
    df_t m_pi_2 = pi_2;
    m_pi_2.sign = 1;

    // radの周期性を利用して範囲を狭める
    df_mod(&a_tmp, &pi2, &a_tmp);

    // -pi/2<=a<=pi/2の範囲に収める
    if (df_compare(&a_tmp, &pi) >= 0)
    {
        df_sub(&a_tmp, &pi, &a_tmp);
        a_tmp.sign ^= 1;
    }
    else if (df_compare(&a_tmp, &m_pi) <= 0)
    {
        df_add(&a_tmp, &pi, &a_tmp);
        a_tmp.sign ^= 1;
    }

    if (df_compare(&a_tmp, &pi_2) > 0)
    {
        df_sub(&pi, &a_tmp, &a_tmp);
    }
    else if (df_compare(&a_tmp, &m_pi_2) < 0)
    {
        df_sub(&m_pi, &a_tmp, &a_tmp);
    }

    int_to_df(0, &tmp2);

    // 1/(2i+1)!
    df_t c0[10] = {
        {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0},
        {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1}, -1},
        {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8}, -3},
        {0, {3, 1, 4, 8, 9, 6, 2, 1, 4, 8, 9, 6, 2, 1, 4, 8, 9, 1}, -4},
        {0, {7, 0, 9, 8, 5, 8, 9, 3, 2, 2, 9, 1, 3, 7, 5, 5, 7, 2}, -6},
        {0, {8, 8, 1, 7, 1, 4, 4, 5, 8, 3, 8, 0, 1, 2, 5, 0, 5, 2}, -8},
        {0, {6, 4, 1, 6, 1, 2, 8, 6, 3, 8, 3, 4, 0, 9, 5, 0, 6, 1}, -10},
        {0, {8, 4, 6, 1, 8, 9, 1, 8, 1, 3, 7, 3, 6, 1, 7, 4, 6, 7}, -13},
        {0, {6, 7, 0, 2, 5, 5, 4, 3, 4, 5, 2, 7, 5, 4, 1, 1, 8, 2}, -15},
        {0, {2, 7, 9, 2, 3, 4, 2, 6, 6, 4, 2, 5, 3, 6, 0, 2, 2, 8}, -18}};

    for (int8_t i = 0; i < loop; i++)
    {
        df_pow_int(&a_tmp, 2 * i + 1, &tmp); // a^(2i+1)
        df_mul(&tmp, &c0[i], &tmp);          // a^(2i+1)/(2i+1)!
        if (i % 2 == 0)
        {
            df_add(&tmp2, &tmp, &tmp2);
        }
        else
        {
            df_sub(&tmp2, &tmp, &tmp2);
        }
    }
    if (df_is_zero(&tmp2))
    {
        tmp2.sign = 0;
    }

    if (df_compare(&tmp2, &one) > 0)
    {
        tmp2 = one;
    }
    if (df_compare(&tmp2, &mone) < 0)
    {
        tmp2 = mone;
    }

    *result = tmp2;
}

// cos(a)
void df_cos(df_t *a, df_t *result)
{
    df_t pi_2 = DF_PId2;
    df_t right_angle = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9}, 1};
    df_t a_tmp = *a;
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_add(&a_tmp, &right_angle, &a_tmp);
    }
    else
    {
        df_add(&a_tmp, &pi_2, &a_tmp);
    }

    df_sin(&a_tmp, &a_tmp);
    if (df_is_zero(&a_tmp))
    {
        a_tmp.sign = 0;
    }
    *result = a_tmp;
}

// tan(a)
void df_tan(df_t *a, df_t *result)
{
    df_t tmp, tmp2, tmp3;
    df_t a_tmp = *a;

    df_sin(&a_tmp, &tmp);
    df_cos(&a_tmp, &tmp2);
    df_div(&tmp, &tmp2, &tmp3);
    *result = tmp3;
}

// arcsin(a)
void df_asin(df_t *a, df_t *result)
{
    double x_f;
    df_t y;
    df_t minus_one = DF_m1;
    df_t one = DF_1;

    if (df_compare(a, &minus_one) < 0 || df_compare(a, &one) > 0)
    {
        df_error(result);
        return;
    }

    df_to_double(a, &x_f);
    double_to_df(asin(x_f), &y);
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(&y, &y);
    }

    *result = y;
}
// void df_asin(df_t *a, df_t *result)
// {
//     const uint8_t loop = 9;
//     df_t a_tmp, tmp, tmp2, term1, a_sq;
//     a_tmp = *a;

//     df_t minus_one = DF_m1;
//     df_t one = DF_1;

//     if (df_compare(&a_tmp, &minus_one) < 0 || df_compare(a, &one) > 0)
//     {
//         df_error(result);
//         return;
//     }

//     df_mul(&a_tmp, &a_tmp, &a_sq);
//     term1 = a_tmp;
//     tmp = a_tmp;

//     //(2*i-1)/(2 *i+ 1)
//     df_t c0[10] = {
//         {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, -1},
//         {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6}, -1},
//         {0, {6, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7}, -1},
//         {0, {8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7}, -1},
//         {0, {2, 8, 1, 8, 1, 8, 1, 8, 1, 8, 1, 8, 1, 8, 1, 8, 1, 8}, -1},
//         {0, {4, 5, 1, 6, 4, 8, 3, 5, 1, 6, 4, 8, 3, 5, 1, 6, 4, 8}, -1},
//         {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8}, -1},
//         {0, {8, 5, 1, 3, 6, 2, 5, 0, 1, 2, 4, 8, 6, 3, 7, 4, 9, 8}, -1},
//         {0, {2, 6, 7, 4, 0, 9, 1, 6, 7, 4, 0, 9, 1, 6, 7, 4, 0, 9}, -1}};

//     // 1/(2 *i+ 1)
//     df_t c1[10] = {
//         {0, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, -1},
//         {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}, -1},
//         {0, {7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1, 7, 5, 8, 2, 4, 1}, -1},
//         {0, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, -1},
//         {0, {1, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9, 0, 9}, -2},
//         {0, {1, 3, 2, 9, 6, 7, 0, 3, 2, 9, 6, 7, 0, 3, 2, 9, 6, 7}, -2},
//         {0, {7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6}, -2},
//         {0, {9, 5, 0, 7, 4, 6, 7, 1, 1, 4, 9, 2, 5, 3, 2, 8, 8, 5}, -2},
//         {0, {1, 1, 2, 4, 8, 6, 3, 7, 4, 9, 8, 7, 5, 1, 3, 6, 2, 5}, -2}};

//     for (int8_t i = 0; i < loop; i++)
//     {
//         df_mul(&a_sq, &c0[i], &tmp2);
//         df_mul(&tmp2, &term1, &term1);
//         df_mul(&term1, &c1[i], &tmp2);
//         df_add(&tmp2, &tmp, &tmp);
//     }

//     if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
//     {
//         df_rad_to_deg(&tmp, &tmp);
//     }

//     if (df_is_zero(&tmp))
//     {
//         tmp.sign = 0;
//     }
// }

// arccos(a)
void df_acos(df_t *a, df_t *result)
{
    double x_f;
    df_t y;
    df_t minus_one = DF_m1;
    df_t one = DF_1;

    if (df_compare(a, &minus_one) < 0 || df_compare(a, &one) > 0)
    {
        df_error(result);
        return;
    }

    df_to_double(a, &x_f);
    double_to_df(acos(x_f), &y);
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(&y, &y);
    }
    *result = y;
}

// arctan(a)
void df_atan(df_t *a, df_t *result)
{
    double x_f;
    df_t y;
    df_to_double(a, &x_f);
    double_to_df(atan(x_f), &y);
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(&y, &y);
    }
    *result = y;
}

// pi
df_t df_pi()
{
    df_t pi = DF_PI;

    return pi;
}

// deg to rad
void df_deg_to_rad(df_t *deg, df_t *result)
{
    df_t tmp, pi;
    tmp = *deg;
    pi = df_pi();
    df_t one_eighty = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1}, 2};
    df_div(&tmp, &one_eighty, &tmp);
    df_mul(&tmp, &pi, result);
}

// rad to deg
void df_rad_to_deg(df_t *rad, df_t *result)
{
    df_t tmp, pi;
    pi = df_pi();
    df_t one_eighty = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 1}, 2};
    df_div(rad, &pi, &tmp);
    df_mul(&tmp, &one_eighty, result);
}

// e
df_t df_e()
{
    df_t e = DF_E;
    return e;
}

// (a*b)/(a+b)
void df_mul_over_sum(df_t *a, df_t *b, df_t *result)
{
    df_t tmp, tmp2;
    df_add(a, b, &tmp);
    df_mul(a, b, &tmp2);
    df_div(&tmp2, &tmp, result);
}

// fc(r,c)
void df_fc_rc(df_t *r, df_t *c, df_t *result)
{
    df_t tmp, pi;
    df_t two = DF_2;
    pi = df_pi();
    df_mul(r, c, &tmp);
    df_mul(&tmp, &two, &tmp);
    df_mul(&tmp, &pi, &tmp);
    df_inv(&tmp, result);
}

// fc(l,c)
void df_fc_lc(df_t *l, df_t *c, df_t *result)
{
    df_t tmp, pi;
    df_t two = DF_2;
    pi = df_pi();
    df_mul(l, c, &tmp);
    df_sqrt(&tmp, &tmp);
    df_mul(&tmp, &two, &tmp);
    df_mul(&tmp, &pi, &tmp);
    df_inv(&tmp, result);
}

// 表示桁数の一桁下で四捨五入
void df_round(df_t *a, df_t *result)
{
    *result = *a;

    if (result->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT - 1] >= 5)
    {
        result->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT]++;
        result->mantissa[0] = 0;
        result->mantissa[1] = 0;
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; i++)
        {
            result->mantissa[i] = 0;
        }

        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            if (result->mantissa[i] >= 10)
            {
                result->mantissa[i + 1]++;
                result->mantissa[i] -= 10;
            }
            else
            {
                return;
            }
        }
        result->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] = 1;
        result->exponent++;
    }
}

// 表示モードの設定
df_string_mode_t df_string_mode = DF_STRING_MODE_SCIENTIFIC;
void set_df_string_mode(df_string_mode_t mode)
{
    df_string_mode = mode;
}

df_string_mode_t get_df_string_mode()
{
    return df_string_mode;
}

// 十進浮動小数点数を文字列に変換
void df_to_string(df_t *a, char *str)
{
    df_string_mode_t mode = df_string_mode;
    int8_t i = 0;
    if (a->exponent <= -100 || a->exponent >= 100)
    {
        sprintf(str, "OverFlow");
        int_to_df(0, a); // 入力変数を直接ゼロにしているので注意。
        return;
    }
    if (df_is_zero(a) && a->sign == 1)
    {
        sprintf(str, "ERROR");
        int_to_df(0, a); // 入力変数を直接ゼロにしているので注意。
        return;
    }
    if (a->sign)
    {
        str[i++] = '-';
    }

    if (mode == DF_STRING_MODE_ENGINEERING)
    {
        str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] + '0';
        str[i++] = '.';
        for (int8_t j = DECIMAL_FLOAT_MANTISSA_SIZE - 2; j >= DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; j--)
        {
            str[i++] = a->mantissa[j] + '0';
        }
        str[i++] = 'E';

        sprintf(&str[i], "%d", a->exponent);
    }
    else if (mode == DF_STRING_MODE_SCIENTIFIC)
    {
        int16_t new_exp = a->exponent;
        int16_t mod = new_exp % 3;
        if (mod == 0)
        {
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] + '0';
            str[i++] = '.';
            for (int8_t j = DECIMAL_FLOAT_MANTISSA_SIZE - 2; j >= DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; j--)
            {
                str[i++] = a->mantissa[j] + '0';
            }
            str[i++] = 'E';

            sprintf(&str[i], "%d", new_exp);
        }
        else if (mod == 1 || mod == -2)
        {
            new_exp -= 1;
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] + '0';
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 2] + '0';
            str[i++] = '.';
            for (int8_t j = DECIMAL_FLOAT_MANTISSA_SIZE - 3; j >= DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; j--)
            {
                str[i++] = a->mantissa[j] + '0';
            }
            str[i++] = 'E';

            sprintf(&str[i], "%d", new_exp);
        }
        else if (mod == 2 || mod == -1)
        {
            new_exp -= 2;
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] + '0';
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 2] + '0';
            str[i++] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 3] + '0';
            str[i++] = '.';
            for (int8_t j = DECIMAL_FLOAT_MANTISSA_SIZE - 4; j >= DECIMAL_FLOAT_MANTISSA_SIZE - DECIMAL_FLOAT_DISPLAY_DIGIT; j--)
            {
                str[i++] = a->mantissa[j] + '0';
            }
            str[i++] = 'E';

            sprintf(&str[i], "%d", new_exp);
        }
    }
    else
    {
        sprintf(str, "ERROR");
    }
}

// 数値文字列を正規化した数値文字列に変換
void normalize_df_string(char *input, char *output)
{
    int8_t len = strlen(input);
    int8_t decimal_pos = -1;
    int8_t first_digit_pos = -1;
    int16_t exponent = 0;
    int8_t e_pos = -1;
    int8_t is_negative = 0;

    // 符号の処理
    if (input[0] == '-')
    {
        is_negative = 1;
        input++;
        len--;
    }

    // 小数点の位置、最初の非ゼロの桁、および 'e' または 'E' の位置を見つける
    for (int8_t i = 0; i < len; i++)
    {
        if (input[i] == '.')
        {
            decimal_pos = i;
        }
        if (first_digit_pos == -1 && input[i] >= '1' && input[i] <= '9')
        {
            first_digit_pos = i;
        }
        if (input[i] == 'e' || input[i] == 'E')
        {
            e_pos = i;
            break;
        }
    }

    // 指数部分の処理を追加する
    if (e_pos != -1)
    {
        exponent = atoi(input + e_pos + 1);
        len = e_pos; // 指数部分は一旦無視する
    }

    // 小数点がない場合は末尾にあるものとして扱う
    if (decimal_pos == -1)
    {
        decimal_pos = len;
    }

    // 入力がゼロかどうかを確認する
    bool is_zero = true;
    for (int8_t i = 0; i < len; i++)
    {
        if (input[i] != '0' && input[i] != '.')
        {
            is_zero = false;
            break;
        }
    }

    if (is_zero)
    {
        sprintf(output, "0e+%d", exponent);
        return;
    }

    // 正規化のための指数を計算する
    int8_t normalization_exponent = decimal_pos - first_digit_pos - 1;
    if (decimal_pos < first_digit_pos)
    {
        normalization_exponent++;
    }

    exponent += normalization_exponent;

    // 出力文字列を構築する
    int8_t output_pos = 0;
    if (is_negative)
    {
        output[output_pos++] = '-';
    }

    output[output_pos++] = input[first_digit_pos];

    if (first_digit_pos + 1 < len && input[first_digit_pos + 1] != '.')
    {
        output[output_pos++] = '.';
    }

    for (int8_t i = first_digit_pos + 1; i < len; i++)
    {
        if (input[i] != '.')
        {
            output[output_pos++] = input[i];
        }
    }

    output[output_pos++] = 'e';

    if (exponent >= 0)
    {
        output[output_pos++] = '+';
    }
    else
    {
        output[output_pos++] = '-';
        exponent = -exponent;
    }

    sprintf(output + output_pos, "%d", exponent);
}

// 文字列を十進浮動小数点数に変換
void string_to_df(char *in, df_t *a)
{
    char str[20] = {};
    strcpy(str, in);
    if (strcmp(str, "ERROR") == 0)
    {
        int_to_df(0, a);
        return;
    }
    else if (strcmp(str, "OverFlow") == 0)
    {
        int_to_df(0, a);
        return;
    }

    normalize_df_string(in, str);

    int8_t i = 0;
    if (str[i] == '-')
    {
        a->sign = 1;
        i++;
    }
    else
    {
        a->sign = 0;
    }
    for (int8_t j = 0; j < DECIMAL_FLOAT_MANTISSA_SIZE; j++)
    {
        if (str[i] != 'e' && str[i] != 'E')
        {
            if (str[i] == '.')
            {
                i++;
            }

            a->mantissa[j] = str[i] - '0';
            i++;
        }
        else
        {
            a->mantissa[j] = 0;
        }
    }
    // 順序反転
    for (int8_t j = 0; j < DECIMAL_FLOAT_MANTISSA_SIZE / 2; j++)
    {
        int8_t temp = a->mantissa[j];
        a->mantissa[j] = a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - j - 1];
        a->mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - j - 1] = temp;
    }

    i++;
    a->exponent = (int16_t)atoi(&str[i]);
}
