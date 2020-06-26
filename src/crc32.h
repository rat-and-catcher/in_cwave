/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      crc32.h -- calculations of standard CRC32
 *      This is a part of CWAVE format
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

/* For each CRC computation, the code must provide
 * the temporary variable ('tcrc'), which initialized
 * by the crc32Init() and updated by crc32update().
 * The final CRC computed by value of tcrc by crc32final.
 */

#if !defined(_crc32_h_)
#define _crc32_h_

#if defined(__cplusplus)
extern "C" {
#endif

/* the polynomial
*/
#define POLYNOMIAL      (0xEDB88320)

/* the struct for temporary data
*/
typedef struct tagTmpCrc32
{
 unsigned xOr;                  // inversion mask
 unsigned temp;                 // temporary value for CRC
} TMP_CRC32;

/* initialization of computation
*/
void crc32init(TMP_CRC32 *t);

/* update CRC by block of data
*/
void crc32update(const void *data, unsigned len, TMP_CRC32 *t);

/* return final CRC by the last 'temp'
*/
unsigned crc32final(TMP_CRC32 *t);


#if defined(__cplusplus)
}
#endif

#endif          // def _crc32_h_

/* the end...
*/

