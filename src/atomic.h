/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      atomic.h -- some atomic data manipulation stuff
 *
 * Copyright (c) 2010-2024, Rat and Catcher Technologies
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

#if !defined(_atomic_h_)
#define _atomic_h_

// NOTE::This file, probably, need to be removed from the project

#include "compatibility/compat_gcc.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <stdint.h>

#include "compatibility/compat_win32_gcc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* atomic int type
*/
typedef          LONG V_INT;                        // "atomic" value
typedef volatile LONG A_INT;                        // variable to contain atomic value

/* atomic write A_INT value
*/
static __inline void aint_write(A_INT *dst, V_INT src)
{
#if defined(ATM_COPY_FAST)
 *dst = src;
#else

 V_INT tmp;

 do
 {
  tmp = *dst;
 }
 while(InterlockedCompareExchange(dst, src, tmp) != tmp);
#endif
}

/* atomic read A_INT value
*/
static __inline V_INT aint_read(/*!!const!!*/ A_INT *src)
{
#if defined(ATM_COPY_FAST)
 return *src;
#else
 return InterlockedCompareExchange(src, 0, 0);
#endif
}

/* atomic write double value
*/
static __inline void adbl_write(volatile double *dst, double src)
{
#if defined(ATM_COPY_FAST)
 *dst = src;
#else

#if defined(__GNUC__)
#pragma GCC diagnostic push
// we are sure, that for x86 "double" not less strictly aligned,
// than WIN32 "LONGLONG"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
// should work -- double value is simple 64 "integer" bits
 volatile LONGLONG *ldst = (volatile LONGLONG *)dst;
 LONGLONG lsrc = *((LONGLONG *)&src);
 LONGLONG tmp;

 do
 {
  tmp = *ldst;
 }
 while(InterlockedCompareExchange64(ldst, lsrc, tmp) != tmp);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
}

/* atomic read double value
*/
static __inline double adbl_read(/*!!const!!*/ volatile double *src)
{
#if defined(ATM_COPY_FAST)
 return *src;
#else

#if defined(__GNUC__)
#pragma GCC diagnostic push
// we are sure, that for x86 "double" not less strictly aligned,
// than WIN32 "LONGLONG"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

 LONGLONG res = InterlockedCompareExchange64(   // should work -- double value is simple 64 "integer" bits
      (volatile LONGLONG *)src
    , 0LL
    , 0LL);

 return *((double *)&res);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
}


#if defined(__cplusplus)
}
#endif

#endif          // def _atomic_h_

/* the end...
*/

