/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      fp_check.c -- checking floating point operations and gather exceptions statictics
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

#include "fp_check.h"


/* initialize / reset the statistics
*/
void except_stats_reset(FP_EXCEPT_STATS *fes)
{
// interlocking don't need
 fes -> cnt_total
    = fes -> cnt_snan
    = fes -> cnt_qnan
    = fes -> cnt_ninf
    = fes -> cnt_nden
    = fes -> cnt_pden
    = fes -> cnt_pinf
    = 0;
}

/* check the double value / expression
*/
double except_stats_check(double val, FP_EXCEPT_STATS *fes)
{
 int fpc = _fpclass(val) & (_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_ND | _FPCLASS_PD | _FPCLASS_PINF);

 if(fpc && fes -> is_enable)
 {
  // really, we don't need InterlockedIncrement() here
  InterlockedIncrement((volatile LONG *)&(fes -> cnt_total));

  if(fpc & _FPCLASS_SNAN)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_snan));
   val = 0.0;
  }

  if(fpc & _FPCLASS_QNAN)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_qnan));
   val = 0.0;
  }

  if(fpc & _FPCLASS_ND)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_nden));
   val = 0.0;
  }

  if(fpc & _FPCLASS_PD)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_pden));
   val = 0.0;
  }

  if(fpc & _FPCLASS_NINF)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_ninf));
   val = -INF_HUGE_VALUE;
  }

  if(fpc & _FPCLASS_PINF)
  {
   InterlockedIncrement((volatile LONG *)&(fes -> cnt_pinf));
   val = INF_HUGE_VALUE;
  }
 }

 return val;
}


/* the end...
*/

