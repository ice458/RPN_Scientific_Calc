#ifndef SWITCH_H
#define	SWITCH_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "definitions.h"

#define K0 208
#define KDOT 200
#define KE 196
#define KSHIFT 194
#define KENTER 193
#define K1 176
#define K2 168
#define K3 164
#define KADD 162
#define KSUB 161
#define K4 144
#define K5 136
#define K6 132
#define KMUL 130
#define KDIV 129
#define K7 112
#define K8 104
#define K9 100
#define KPRI 98
#define KDEL 97
#define KR 80
#define KSWAP 72
#define KSIN 68
#define KCOS 66
#define KTAN 65
#define KSQRT 48
#define KPOW2 40
#define KPOW 36
#define KLOG 34
#define KLN 33
#define KMoS 16
#define KRC 8
#define KF1 4
#define KF2 2
#define KLOGXY 1

    uint8_t key_read();
    void init_sw();

#ifdef	__cplusplus
}
#endif

#endif