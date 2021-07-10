/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      unpack_lsb.h -- unpack some data types by a pointer (LSB only)
 *
 * Copyright (c) 2010-2020, Rat and Catcher Technologies
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

#if !defined(_unpack_lsb_h_)
#define _unpack_lsb_h_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

// the code used own integer types due to historical reasons
typedef uint8_t     U8;
typedef uint16_t    U16;
typedef uint32_t    U32;
typedef uint64_t    U64;

// make the pointer to unsigned byte
#define U8P(aptr)  ((U8 *)(aptr))


/* unpack signed short -- 16 bits
*/
static __inline int16_t unpack_int16(const void *p)
{
 U16 u0 = U8P(p)[0];
 U16 u1 = U8P(p)[1];

 return (int16_t)(u0 | (u1 << 8));
}

/* unpack int -- 24 bits
*/
static __inline int unpack_int24(const void *p)
{
 U32 u0 = U8P(p)[0];
 U32 u1 = U8P(p)[1];
 U32 u2 = U8P(p)[2];

 return ((int)((u0 << 8) | (u1 << 16) | (u2 << 24))) >> 8;      // here we set a sign for int
}

/* unpack int -- 32 bits
*/
static __inline int unpack_int32(const void *p)
{
 U32 u0 = U8P(p)[0];
 U32 u1 = U8P(p)[1];
 U32 u2 = U8P(p)[2];
 U32 u3 = U8P(p)[3];

 return (int)(u0 | (u1 << 8) | (u2 << 16) | (u3 << 24));
}

/* unpack float -- 32 bits
*/
static __inline float unpack_float(const void *p)
{
 U32 u0 = U8P(p)[0];
 U32 u1 = U8P(p)[1];
 U32 u2 = U8P(p)[2];
 U32 u3 = U8P(p)[3];
 union
 {
  float fres;
  U32 ures;
 } res;

 res.ures = u0 | (u1 << 8) | (u2 << 16) | (u3 << 24);

 return res.fres;
}

/* unpack double -- 64 bits
*/
static __inline double unpack_double(const void *p)
{
 U64 u0 = U8P(p)[0];
 U64 u1 = U8P(p)[1];
 U64 u2 = U8P(p)[2];
 U64 u3 = U8P(p)[3];
 U64 u4 = U8P(p)[4];
 U64 u5 = U8P(p)[5];
 U64 u6 = U8P(p)[6];
 U64 u7 = U8P(p)[7];
 union
 {
  double dres;
  U64 ures;
 } res;

 res.ures = u0 | (u1 <<  8) | (u2 << 16) | (u3 << 24) | (u4 << 32)
               | (u5 << 40) | (u6 << 48) | (u7 << 56);

 return res.dres;
}



#if defined(__cplusplus)
}
#endif

#endif                  // def _unpack_lsb_h_

/* the end...
*/

