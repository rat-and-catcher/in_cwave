/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      fp_check.h -- checking floating point operations and gather exceptions statictics
 *
 * Copyright (c) 2010-2023, Rat and Catcher Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the Rat and Catcher Technologies nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RAT AND CATCHER TECHNOLOGIES BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(_fp_check_h_)
#define _fp_check_h_

#include "compatibility/compat_gcc.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <float.h>

// for MinGW only
#include <math.h>
#include <tchar.h>

#include "compatibility/compat_win32_gcc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* the "huge value" to replace +-Infinity
*/
#define INF_HUGE_VALUE  ((double)0xFFFF)                            /* nuge enough for integer samples -- both 16 & 24 bit */

/* floating exception statistics
*/
typedef struct tagFP_EXCEPT_STATS
{
 BOOL is_enable;                                                    // enable exception counting
 volatile unsigned cnt_total;                                       // ## tolal exceptions
 volatile unsigned cnt_snan;                                        // ## signaling NaN
 volatile unsigned cnt_qnan;                                        // ## quiet NaN
 volatile unsigned cnt_ninf;                                        // ## negative infinity
 volatile unsigned cnt_nden;                                        // ## negative denormal
 volatile unsigned cnt_pden;                                        // ## positive denormal
 volatile unsigned cnt_pinf;                                        // ## positive infinity
} FP_EXCEPT_STATS;

/* service functions
*/
// -- initialize / reset the statistics
void except_stats_reset(FP_EXCEPT_STATS *fes);
// -- check the double value / expression
double except_stats_check(double val, FP_EXCEPT_STATS *fes);

/* some corresponding macros
*/
// general case -- abbrivation only
#define FEC(v, s)       except_stats_check((v), (s))

// suppose, that *fes deined as 'FES'
#define FC(v)           except_stats_check((v), FES)


#if defined(__cplusplus)
}
#endif

#endif          // def _fp_check_h_

/* the end...
*/

