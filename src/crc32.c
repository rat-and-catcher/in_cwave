/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      crc32.c -- calculations of standard CRC32
 *      This is a part of CWAVE format
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

#include "crc32.h"

// the  table for CRC32 computations
static uint32_t crc32table_[256] =
{
 0, 0                                           // not initialized
};

/* helper -- initialization of CRC32 computation
*/
static uint32_t ini_(uint32_t temp)
{
 temp = temp >> 8 ^ crc32table_[(uint8_t)temp];
 temp = temp >> 8 ^ crc32table_[(uint8_t)temp];
 temp = temp >> 8 ^ crc32table_[(uint8_t)temp];
 temp = temp >> 8 ^ crc32table_[(uint8_t)temp];
 return temp;
}

/* initialization of computation
*/
void crc32init(TMP_CRC32 *t)
{
// the table
 if(!crc32table_[1])
 {
  int i, j;
  uint32_t temp;

  for(i = 0; i < 256; ++i)
  {
   temp = i;
   for(j = 0; j < 8; ++j)
   {
    temp = (temp >> 1) ^ (-(int32_t)(temp & 1) & POLYNOMIAL);
   }
   crc32table_[i] = temp;
  }
 }

// initial values
 t -> xOr = ~0;
 t -> temp = 0;
}

/* update CRC by block of data
*/
void crc32update(const void *data, unsigned len, TMP_CRC32 *t)
{
 uint8_t *pdata = (uint8_t *)data;

// 1-st 4 bytes take with inversion. If data portion less 4, it prolongated by zeros
 for ( ; t -> xOr && len; --len, ++pdata)
 {
  t -> temp = t -> temp >> 8 | ~*pdata << 24;
  t -> xOr >>= 8;
  if(0 == t -> xOr)
  {
   t -> temp  = ini_(t -> temp);
  }
 }

// "generic" CRC32 computation
 while(len)
 {
  t -> temp = crc32table_[(uint8_t)t -> temp ^ *pdata++] ^ (t -> temp >> 8);
  --len;
 }
}

/* return final CRC by the last 'temp'
*/
uint32_t crc32final(TMP_CRC32 *t)
{
 return ~(t -> xOr? t -> xOr ^ ini_(t -> temp) : t -> temp);
}

/* the end...
*/


