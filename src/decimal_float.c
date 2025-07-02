#include "decimal_float.h"
#include "precomputed_reciprocal_values.h"

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
    if (f == 0)
    {
        a->sign = 0;
        a->exponent = 0;
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            a->mantissa[i] = 0;
        }
        return;
    }

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
    } // シフト量が負の場合
      // これは、大きな数の指数部が小さな数の指数部より小さいときに発生する
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

    if (df_is_zero(a) && df_is_zero(b))
    {
        int_to_df(0, result);
        return;
    }
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
void df_pow_int(df_t *a, int32_t n, df_t *result)
{
    df_t base = *a;
    df_t tmp = DF_1;
    int32_t exp = n;

    // n < 0 の場合の処理
    if (n < 0)
    {
        exp = -n;
        // 底がゼロの場合は無限大になるためエラー
        if (df_is_zero(a))
        {
            df_error(result);
            return;
        }
    }

    // 指数が0の場合は1を返す
    if (exp == 0)
    {
        *result = tmp; // tmp = DF_1
        return;
    }

    // 二分累乗法
    while (exp > 0)
    {
        // 指数が奇数の場合
        if (exp % 2 == 1)
        {
            df_mul(&tmp, &base, &tmp);
        }
        // 指数を半分にする
        exp /= 2;
        if (exp > 0)
        {
            // base = base^2
            df_mul(&base, &base, &base);
        }
    }
    // 負の指数の場合は逆数を取る
    if (n < 0)
    {
        df_inv(&tmp, result);
    }
    else
    {
        *result = tmp;
    }
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
    // これでいいのかわからん無限小数の時、最終桁を必ず1大きくしてしまう
    // 表示する桁より多く計算しているならこれでもいいのでは
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
    if (df_is_zero(a))
    {
        int_to_df(0, result);
        return;
    }

    df_t n, tmp;
    double a_f;
    df_t point_five = DF_05;
    df_t three = DF_3;

    df_to_double(a, &a_f);

    // 初期値の設定
    double_to_df(1.0 / sqrt(a_f), &n);
    // ニュートン法で1/sqrt(a)を求める
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE + 1; i++)
    {
        df_mul(&n, &n, &tmp);
        df_mul(a, &tmp, &tmp);
        df_sub(&three, &tmp, &tmp);
        df_mul(&point_five, &tmp, &tmp);
        df_mul(&n, &tmp, &n);
    }
    df_mul(&n, a, &n);

    // 丸め処理
    // これで良いのかわからん無限小数の時、最終桁を必ず1大きくしてしまう
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
    if (df_is_zero(a))
    {
        int_to_df(0, result);
        return;
    }

    df_t x = *a; // 計算用のコピー
    bool is_negative = x.sign;
    x.sign = 0; // 絶対値で計算

    // --- 初期値の推定 ---
    df_t n = DF_1; // 推定値 y_n
    n.sign = 0;    // 正の値で開始

    // 指数部から大まかな桁を推定: E_new ≈ floor(E_old / 3)
    int16_t initial_exponent = x.exponent / 3;
    // 指数部の剰余に応じて仮数部を調整
    int16_t remainder = x.exponent % 3;
    if (remainder < 0)
    { // 負の指数の場合
        initial_exponent--;
        remainder += 3;
    }

    // 仮数部の初期値 (簡単な推定: 1.0 or 2.0)
    // M * 10^E = (M * 10^rem) * 10^(3*k) -> cbrt ≈ cbrt(M*10^rem) * 10^k
    // M*10^rem は 1 から 1000 未満立方根は 1 から 10 未満
    // 簡単のため、初期値の仮数部を 1.0 とする
    n.exponent = initial_exponent;

    // --- ニュートン法による反復計算 ---
    // y_{n+1} = (1/3) * (2 * y_n + a / y_n^2)
    df_t term1, term2, n_squared, tmp;
    df_t one_third = DF_1_3; // 1/3
    df_t two = DF_2;         // 2

    // 収束判定のための閾値
    df_t epsilon = DF_0;
    epsilon.exponent = n.exponent - DECIMAL_FLOAT_MANTISSA_SIZE - 1; // n のスケールに合わせる
    epsilon.mantissa[0] = 1;

    df_t previous_n = DF_0; // 収束判定用

    // 反復回数の上限
    const int max_iterations = DECIMAL_FLOAT_MANTISSA_SIZE + 5; // df_inv より少し多めに

    for (int i = 0; i < max_iterations; ++i)
    {
        previous_n = n; // 前回の値を保存

        // term1 = 2 * y_n
        df_mul(&two, &n, &term1);

        // n_squared = y_n^2
        df_mul(&n, &n, &n_squared);

        // term2 = a / y_n^2 (ゼロ除算チェック)
        if (df_is_zero(&n_squared))
        {
            df_error(result);
            return;
        }
        df_div(&x, &n_squared, &term2);

        // tmp = term1 + term2 = 2*y_n + a/y_n^2
        df_add(&term1, &term2, &tmp);

        // n = (1/3) * tmp
        df_mul(&one_third, &tmp, &n);

        // 収束判定: |n - previous_n| < epsilon
        df_sub(&n, &previous_n, &tmp);
        tmp.sign = 0; // 絶対値
        // epsilon のスケールを現在の n に合わせる
        epsilon.exponent = n.exponent - DECIMAL_FLOAT_MANTISSA_SIZE - 1;
        if (df_compare(&tmp, &epsilon) <= 0)
        {
            break;
        }
    }

    *result = n; // 計算結果を格納

    // 元の数が負だった場合、結果の符号を負にする
    if (is_negative)
    {
        result->sign = 1;
    }
}

// nth_root(a, n)
void df_nth_root(df_t *a, df_t *n, df_t *result)
{
    // nがゼロの場合はエラー
    if (df_is_zero(n))
    {
        df_error(result);
        return;
    }

    // aがゼロの場合は結果もゼロ
    if (df_is_zero(a))
    {
        int_to_df(0, result);
        return;
    }

    // nが整数かどうかをチェック
    df_t n_rounded;
    df_round_int(n, &n_rounded);
    bool is_n_integer = true;

    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - n->exponent - 1; i++)
    {
        if (n->mantissa[i] != 0)
        {
            is_n_integer = false;
            break;
        }
    }

    // aが負かつnが整数でない場合はエラー
    if (a->sign && !is_n_integer)
    {
        df_error(result);
        return;
    }

    // nが奇数かどうかチェック(nが整数の場合のみ)
    bool is_n_odd = false;
    if (is_n_integer)
    {
        is_n_odd = (n_rounded.mantissa[DECIMAL_FLOAT_MANTISSA_SIZE - 1] % 2 == 1);
    }

    // aが負かつnが偶数の場合はエラー
    if (a->sign && !is_n_odd)
    {
        df_error(result);
        return;
    }

    // aが負かつnが奇数の場合
    if (a->sign && is_n_odd)
    {
        df_t abs_a = *a;
        abs_a.sign = 0; // 絶対値を取る

        df_t n_inv;
        df_inv(n, &n_inv);

        df_pow(&abs_a, &n_inv, result);
        result->sign = 1; // 結果は負
        return;
    }

    // 通常のケース: aが正の場合
    df_t n_inv;
    df_inv(n, &n_inv);
    df_pow(a, &n_inv, result);
}

// exp(x)
void df_exp(df_t *x, df_t *result)
{
    // 特殊ケース処理
    if (df_is_zero(x))
    {
        df_t one = DF_1;
        *result = one;
        return;
    }

    df_t xtmp = *x;
    bool inv_flag = false;

    // 負の指数は e^(-x) = 1/e^x として計算
    if (xtmp.sign == 1)
    {
        xtmp.sign = 0;
        inv_flag = true;
    }

    // 整数部と小数部に分割
    df_t int_part, frac_part;
    df_round_int(&xtmp, &int_part);
    df_sub(&xtmp, &int_part, &frac_part);

    // 整数部の指数を抽出
    int32_t int_exp = 0;
    if (!df_is_zero(&int_part))
    {
        // 整数部を抽出
        for (int i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= DECIMAL_FLOAT_MANTISSA_SIZE - int_part.exponent - 1; i--)
        {
            int_exp = int_exp * 10 + int_part.mantissa[i];
        }
    }

    // e^1の計算または定数として用意
    df_t e_const = DF_E;

    // 小数部の指数計算 (テイラー級数)
    df_t frac_result = DF_1;     // 1で初期化
    const uint8_t max_loop = 100; // 最大反復回数

    if (!df_is_zero(&frac_part))
    {
        df_t sum = DF_1;                                                                                              // 結果の和
        df_t term = DF_1;                                                                                             // 各項
        df_t epsilon = DF_0;
        epsilon.exponent = -DECIMAL_FLOAT_MANTISSA_SIZE - 2;
        epsilon.mantissa[0] = 1;
        df_t previous_sum = DF_0;

        // k! と x^k/k! の計算
        for (uint8_t i = 1; i <= max_loop; i++)
        {
            // term = term * x / k を計算
            df_mul(&term, &frac_part, &term);
            df_mul(&term, (df_t *)&precomputed_reciprocal[i], &term);

            // 和を更新
            df_add(&sum, &term, &sum);

            // 収束判定: 前回の和との差が閾値より小さい場合は終了
            df_t diff;
            df_sub(&sum, &previous_sum, &diff);
            diff.sign = 0; // 絶対値を取る

            if (df_compare(&diff, &epsilon) <= 0)
            {
                break;
            }

            previous_sum = sum;
        }

        frac_result = sum;
    }

    // 整数部の指数計算 (e^n) - 二分累乗法
    df_t int_result = DF_1; // 1で初期化

    if (int_exp > 0)
    {
        df_t base = e_const;

        while (int_exp > 0)
        {
            if (int_exp % 2 == 1)
            {
                df_mul(&int_result, &base, &int_result);
            }
            int_exp /= 2;

            if (int_exp > 0)
            {
                df_mul(&base, &base, &base);
            }
        }
    }

    // e^x = e^(int_part) * e^(frac_part)
    df_mul(&int_result, &frac_result, result);

    // 負の指数の場合は逆数を取る
    if (inv_flag)
    {
        df_t tmp = *result;
        df_inv(&tmp, result);
    }
}

// ln(x)
// 計算には範囲縮小 (10^m を利用) と artanh((x'-1)/(x'+1)) の級数展開を利用
void df_ln(df_t *x, df_t *result)
{
    if (x->sign || df_is_zero(x))
    {
        df_error(result);
        return;
    }

    df_t one = DF_1;
    // ln(1) = 0 の場合の処理
    if (df_compare(x, &one) == 0)
    {
        int_to_df(0, result);
        return;
    }

    // 範囲縮小
    // x' = x / 10^m が 1 に近くなるように m を決定
    // m は元の数の指数部を使用
    int16_t m = x->exponent;
    df_t x_prime = *x;
    x_prime.exponent = 0; // x' = x / 10^m を作成

    // ln(x') の級数計算 (artanh 級数)
    // y = (x' - 1) / (x' + 1)
    df_t y, numerator, denominator;
    df_sub(&x_prime, &one, &numerator);
    df_add(&x_prime, &one, &denominator);
    df_div(&numerator, &denominator, &y);

    df_t sum = y; // 級数の最初の項
    df_t term = y;
    df_t y_squared;
    df_mul(&y, &y, &y_squared); // 次の項を計算するための y^2

    // 収束判定のための閾値
    df_t epsilon = DF_0;
    epsilon.exponent = -DECIMAL_FLOAT_MANTISSA_SIZE - 2;
    epsilon.mantissa[0] = 1;

    df_t term_div_i;
    df_t i_df;
    df_t previous_term_abs = DF_1; // 収束判定用 (前回の項の絶対値)
    term.sign = 0;                 // 絶対値を取る

    // 級数展開: y + y^3/3 + y^5/5 + ...
    const int max_iterations = 100;
    for (int32_t i = 3, iter_count = 0; iter_count < max_iterations; i += 2, ++iter_count)
    {
        // term = term * y^2
        df_mul(&term, &y_squared, &term);

        // i_df = i (ループカウンタを df_t に変換)
        int_to_df(i, &i_df);

        // term_div_i = term / i
        // df_div(&term, &i_df, &term_div_i);
        df_mul(&term, (df_t *)&precomputed_reciprocal[i], &term_div_i);

        // sum = sum + term_div_i (和を更新)
        df_add(&sum, &term_div_i, &sum);

        // 収束判定: 今回追加した項の絶対値が閾値以下か？
        term_div_i.sign = 0; // 絶対値を取る
        if (df_compare(&term_div_i, &epsilon) <= 0)
        {
            // さらに、前回の項より十分小さくなっているかも確認（振動を防ぐ）
            df_t diff_term;
            df_sub(&previous_term_abs, &term_div_i, &diff_term);
            diff_term.sign = 0;
            if (df_compare(&diff_term, &epsilon) > 0)
            {
                previous_term_abs = term_div_i; // 更新
            }
            else
            {
                break;
            }
        }
        else
        {
            previous_term_abs = term_div_i; // 更新
        }
    }

    // ln(x') = 2 * sum を計算
    df_t ln_x_prime;
    df_t two = DF_2;
    df_mul(&two, &sum, &ln_x_prime);

    // --- 結果の結合 ---
    // ln(x) = ln(x') + m * ln(10) を計算
    df_t m_df;
    int_to_df(m, &m_df); // 整数 m を df_t に変換

    df_t m_ln10 = DF_LN10;
    df_mul(&m_df, &m_ln10, &m_ln10); // m * ln(10) を計算

    df_add(&ln_x_prime, &m_ln10, result); // ln(x') + m * ln(10)
}

// log10(x)
void df_log10(df_t *x, df_t *result)
{
    df_t tmp;
    df_t ten = DF_10;
    static df_t ln10inv;
    bool isfirst = true;
    if (x->sign)
    {
        result->sign = 1;
        result->exponent = 0;
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE; i++)
        {
            result->mantissa[i] = 0;
        }
        return;
    }

    if (isfirst)
    {
        df_ln(&ten, &ln10inv);
        df_inv(&ln10inv, &ln10inv);
        isfirst = false;
    }
    df_ln(x, &tmp);
    df_mul(&tmp, &ln10inv, result);
}

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
    if (df_is_zero(a))
    {
        // 0^0は未定義
        if (df_is_zero(b))
        {
            df_error(result);
            return;
        }
        // 0^bは0（bが正の場合）
        if (b->sign == 0)
        {
            int_to_df(0, result);
            return;
        }
        // 0^bは定義されていない（bが負の場合）
        else
        {
            df_error(result);
            return;
        }
    }

    // bが整数かどうかを確認
    bool is_integer = true;
    for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - b->exponent - 1; i++)
    {
        if (b->mantissa[i] != 0)
        {
            is_integer = false;
            break;
        }
    }

    // bが整数の場合、整数べき乗を計算
    if (is_integer && (b->exponent >= 0 && b->exponent < 9))
    {
        // 整数への変換
        int32_t n = 0;
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= DECIMAL_FLOAT_MANTISSA_SIZE - b->exponent - 1; i--)
        {
            n = n * 10 + b->mantissa[i];
        }
        if (b->sign)
        {
            n = -n;
        }

        df_pow_int(a, n, result);
        return;
    }

    // bが整数でないときかつ、aが負のときはエラー
    if (a->sign)
    {
        df_error(result);
        return;
    }

    // a > 0の場合、a^b = e^(b*ln(a))で計算
    df_t ln_a, b_ln_a;

    df_ln(a, &ln_a);           // ln(a)を計算
    df_mul(b, &ln_a, &b_ln_a); // b*ln(a)を計算
    df_exp(&b_ln_a, result);   // e^(b*ln(a))を計算
}

// pow10(x)
void df_pow10(df_t *x, df_t *result)
{
    df_t one = DF_1;
    // 特殊ケース: x = 0 -> 10^0 = 1
    if (df_is_zero(x))
    {
        *result = one;
        return;
    }

    // x が整数の場合の最適化 (df_pow_int を利用)
    // 整数かどうかをチェック
    bool is_integer = true;
    // 指数が負の場合や、仮数部の小数部分がゼロでない場合は整数ではない
    if (x->exponent < 0)
    {
        is_integer = false;
    }
    else
    {
        // 仮数部の小数部分（指数部より下の桁）をチェック
        for (int8_t i = 0; i < DECIMAL_FLOAT_MANTISSA_SIZE - x->exponent - 1; i++)
        {
            if (x->mantissa[i] != 0)
            {
                is_integer = false;
                break;
            }
        }
    }

    // 整数であり、int32_t の範囲に収まる場合
    // 指数が大きすぎると int32_t に収まらない可能性があるため制限を加える
    // 例: DECIMAL_FLOAT_MANTISSA_SIZE=18 の場合、9桁程度まで安全
    if (is_integer && x->exponent < 9)
    {
        int32_t n = 0;
        // 仮数部の整数部分を int32_t に変換
        for (int8_t i = DECIMAL_FLOAT_MANTISSA_SIZE - 1; i >= DECIMAL_FLOAT_MANTISSA_SIZE - x->exponent - 1; i--)
        {
            // オーバーフローチェック (簡易)
            if (n > (INT32_MAX - x->mantissa[i]) / 10)
            {
                // オーバーフローする場合は通常計算にフォールバック
                is_integer = false;
                break;
            }
            n = n * 10 + x->mantissa[i];
        }

        if (is_integer)
        {
            if (x->sign)
            {
                n = -n;
            }
            df_t ten = DF_10;
            df_pow_int(&ten, n, result);
            return;
        }
    }

    // 一般的なケース: 10^x = e^(x * ln(10))
    df_t ln10 = DF_LN10;
    df_t x_ln10;

    // x * ln(10) を計算
    df_mul(x, &ln10, &x_ln10);

    // e^(x * ln(10)) を計算
    df_exp(&x_ln10, result);
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
    df_t one = DF_1;
    df_t minus_one = DF_m1;
    df_t zero = DF_0;
    df_t pi_div_2 = DF_PId2;
    df_t minus_pi_div_2 = DF_PId2;
    minus_pi_div_2.sign = 1;

    // 1. 定義域チェック [-1, 1]
    if (df_compare(a, &minus_one) < 0 || df_compare(a, &one) > 0)
    {
        df_error(result);
        return;
    }

    // 2. 特殊値処理
    if (df_compare(a, &zero) == 0)
    {
        *result = zero;
        return;
    }
    if (df_compare(a, &one) == 0)
    {
        if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
        {
            df_rad_to_deg(&pi_div_2, result);
        }
        else
        {
            *result = pi_div_2;
        }
        return;
    }
    if (df_compare(a, &minus_one) == 0)
    {
        if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
        {
            df_rad_to_deg(&minus_pi_div_2, result);
        }
        else
        {
            *result = minus_pi_div_2;
        }
        return;
    }

    // 3. 対称性利用 asin(-x) = -asin(x)
    df_t x = *a;
    bool result_is_negative = false;
    if (x.sign)
    {
        x.sign = 0; // 絶対値を取る
        result_is_negative = true;
    }

    // 4. 収束高速化: |x| > 1/sqrt(2) の場合
    // asin(x) = pi/2 - asin(sqrt(1-x^2)) を利用
    // 1/sqrt(2) ~= 0.707
    df_t one_div_sqrt2 = {0, {4, 2, 5, 7, 4, 5, 6, 8, 1, 1, 8, 7, 6, 0, 1, 7, 0, 7}, -1}; // 事前計算値
    bool use_complement = false;
    if (df_compare(&x, &one_div_sqrt2) > 0)
    {
        use_complement = true;
        df_t x_squared, one_minus_x_squared;
        df_mul(&x, &x, &x_squared);                     // x^2
        df_sub(&one, &x_squared, &one_minus_x_squared); // 1 - x^2
        df_sqrt(&one_minus_x_squared, &x);              // sqrt(1 - x^2)
    }

    // 5. 級数計算ループ
    // asin(x) = Σ [ (2n)! / ( (2^n * n!)^2 * (2n+1) ) ] * x^(2n+1)
    // 漸化式: term_n = term_{n-1} * ( (2n-1)^2 / ( (2n) * (2n+1) ) ) * x^2
    df_t sum = x; // 最初の項 (n=0)
    df_t term = x;
    df_t x_squared;
    df_mul(&x, &x, &x_squared); // x^2 を事前計算

    // 収束判定のための閾値
    df_t epsilon = DF_0;
    epsilon.exponent = -DECIMAL_FLOAT_MANTISSA_SIZE - 2;
    epsilon.mantissa[0] = 1;

    df_t two_n_minus_1, two_n, two_n_plus_1;
    df_t factor_num, factor_den, factor, term_prev;

    const int max_iterations = DECIMAL_FLOAT_MANTISSA_SIZE * 2; // 安全のための上限

    for (int n = 1; n < max_iterations; ++n)
    {
        term_prev = term; // 前の項を保存

        // 係数の計算 (2n-1)^2 / ( (2n) * (2n+1) )
        int_to_df(2 * n - 1, &two_n_minus_1);
        int_to_df(2 * n, &two_n);
        int_to_df(2 * n + 1, &two_n_plus_1);

        df_mul(&two_n_minus_1, &two_n_minus_1, &factor_num); // (2n-1)^2
        df_mul(&two_n, &two_n_plus_1, &factor_den);          // (2n) * (2n+1)
        df_div(&factor_num, &factor_den, &factor);           // 係数部分

        // term = term * factor * x^2
        df_mul(&term, &factor, &term);
        df_mul(&term, &x_squared, &term);

        // sum = sum + term
        df_add(&sum, &term, &sum);

        // 収束判定: 今回加えた項の絶対値が閾値以下か？
        df_t diff;
        df_sub(&term, &term_prev, &diff); // 差分ではなく項自体の大きさで判定
        diff.sign = 0;                    // 絶対値
        if (df_compare(&diff, &epsilon) <= 0)
        {
            break;
        }
    }

    // 6. 結果の調整
    df_t final_result = sum;

    // 収束高速化した場合: pi/2 - result
    if (use_complement)
    {
        df_sub(&pi_div_2, &final_result, &final_result);
    }

    // 対称性利用した場合: 符号反転
    if (result_is_negative)
    {
        final_result.sign = 1;
    }

    // 角度モード変換 (DEG)
    if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(&final_result, result);
    }
    else
    {
        *result = final_result;
    }

    // 結果がゼロに近い場合は符号を正にする
    if (df_is_zero(result))
    {
        result->sign = 0;
    }
}

// arccos(a)
void df_acos(df_t *a, df_t *result)
{
    // 定数定義
    df_t one = DF_1;
    df_t minus_one = DF_m1;
    df_t zero = DF_0;
    df_t pi_div_2 = DF_PId2;
    df_t pi = DF_PI;

    // 1. 定義域チェック [-1, 1]
    if (df_compare(a, &minus_one) < 0 || df_compare(a, &one) > 0)
    {
        df_error(result);
        return;
    }

    // 2. 特殊値処理
    if (df_compare(a, &one) == 0) // acos(1) = 0
    {
        *result = zero;
        // 角度モードに関わらず 0
        return;
    }
    if (df_compare(a, &minus_one) == 0) // acos(-1) = pi
    {
        // 角度モードに応じて pi または 180度を返す
        if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
        {
            df_rad_to_deg(&pi, result); // pi を度に変換 (180)
        }
        else
        {
            *result = pi;
        }
        return;
    }
    if (df_compare(a, &zero) == 0) // acos(0) = pi/2
    {
        // 角度モードに応じて pi/2 または 90度を返す
        if (get_df_angle_mode() == DF_ANGLE_MODE_DEG)
        {
            df_rad_to_deg(&pi_div_2, result); // pi/2 を度に変換 (90)
        }
        else
        {
            *result = pi_div_2;
        }
        return;
    }

    // 3. 関係式 acos(x) = pi/2 - asin(x) を利用
    df_t asin_result;

    // df_asin は内部で角度モード変換を行うため、
    // ここで一時的に角度モードを RAD に設定し、asin(x) をラジアンで取得する
    df_angle_mode_t original_mode = get_df_angle_mode();
    set_df_angle_mode(DF_ANGLE_MODE_RAD); // df_asin がラジアンで計算するように強制

    df_asin(a, &asin_result); // asin(a) をラジアンで計算

    // 角度モードを元に戻す
    set_df_angle_mode(original_mode);

    // acos(a) = pi/2 - asin(a) を計算 (結果はラジアン)
    df_sub(&pi_div_2, &asin_result, result);

    // 4. 最終的な角度モード変換
    // 元のモードが DEG なら、ラジアンで計算した結果を度に変換する
    if (original_mode == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(result, result); // 結果を度に変換
    }

    // 結果がゼロに近い場合は符号を正にする
    if (df_is_zero(result))
    {
        result->sign = 0;
    }
}

// arctan(a)
// 関係式 atan(x) = asin(x / sqrt(1 + x^2)) を利用
// または |x| > 1 の場合は atan(x) = +/- pi/2 - atan(1/x) を利用
void df_atan(df_t *a, df_t *result)
{
    // 定数定義
    df_t one = DF_1;
    df_t zero = DF_0;
    df_t pi_div_2 = DF_PId2;
    df_t minus_pi_div_2 = DF_PId2;
    minus_pi_div_2.sign = 1;

    // 1. 特殊値処理: atan(0) = 0
    if (df_is_zero(a))
    {
        *result = zero;
        return;
    }

    // 2. 対称性利用: atan(-x) = -atan(x)
    df_t x = *a;
    bool result_is_negative = false;
    if (x.sign)
    {
        x.sign = 0; // 絶対値で計算
        result_is_negative = true;
    }

    // 3. |x| > 1 の場合の処理
    bool use_complement = false;
    if (df_compare(&x, &one) > 0)
    {
        use_complement = true;
        df_inv(&x, &x); // x = 1/x を計算 (これにより |x| < 1 となる)
    }

    // 4. |x| <= 1 の場合の計算: atan(x) = asin(x / sqrt(1 + x^2))
    df_t x_squared, one_plus_x_squared, sqrt_val, arg, asin_result;

    df_mul(&x, &x, &x_squared);                    // x^2
    df_add(&one, &x_squared, &one_plus_x_squared); // 1 + x^2
    df_sqrt(&one_plus_x_squared, &sqrt_val);       // sqrt(1 + x^2)

    if (df_is_zero(&sqrt_val))
    {
        df_error(result);
        return;
    }
    df_div(&x, &sqrt_val, &arg); // arg = x / sqrt(1 + x^2)

    // df_asin をラジアンモードで呼び出す
    df_angle_mode_t original_mode = get_df_angle_mode();
    set_df_angle_mode(DF_ANGLE_MODE_RAD); // asinの結果をラジアンで得るため

    df_asin(&arg, &asin_result); // asin(arg) をラジアンで計算

    set_df_angle_mode(original_mode); // 角度モードを元に戻す

    // 5. 結果の調整
    df_t final_result = asin_result; // |x| <= 1 の場合の atan(x) [rad]

    // |元のx| > 1 だった場合: +/- pi/2 - atan(1/x)
    if (use_complement)
    {
        // atan(1/x) は asin_result として計算済み
        if (result_is_negative) // 元の x < -1 の場合
        {
            df_sub(&minus_pi_div_2, &asin_result, &final_result); // -pi/2 - atan(1/|x|)
        }
        else // 元の x > 1 の場合
        {
            df_sub(&pi_div_2, &asin_result, &final_result); // pi/2 - atan(1/x)
        }
        // use_complement が true の場合、result_is_negative はここで処理済みなのでリセット
        result_is_negative = false;
    }

    // 元の入力が負だった場合 (かつ use_complement で処理されなかった場合)
    if (result_is_negative)
    {
        final_result.sign = 1;
    }

    // 6. 最終的な角度モード変換
    if (original_mode == DF_ANGLE_MODE_DEG)
    {
        df_rad_to_deg(&final_result, result); // 結果を度に変換
    }
    else
    {
        *result = final_result; // 結果はラジアン
    }

    // 結果がゼロに近い場合は符号を正にする
    if (df_is_zero(result))
    {
        result->sign = 0;
    }
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
        int_to_df(0, a); // 入力変数を直接ゼロにしているので注意
        return;
    }
    if (df_is_zero(a) && a->sign == 1)
    {
        sprintf(str, "ERROR");
        int_to_df(0, a); // 入力変数を直接ゼロにしているので注意
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
