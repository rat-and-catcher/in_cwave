/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      lpf_hilbert_quad.c -- all about making analitic signal via HB LPF
 *      The main idea of this non-trivial transform algorithm taken from
 *      Serge Bahurin, dsplib.ru / dsplib.org. Lots of thanks for him from
 *      Rat and Catcher technologies.
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

#include <stdlib.h>

#include "cmalloc.h"
#include "lpf_hilbert_quad.h"

/* create the converter by IIR_COEFF
*/
LPF_HILBERT_QUAD *hq_rp_create(const RP_IIR_FILTER_DESCR *fdescr, BOOL is_kahan)
{
 LPF_HILBERT_QUAD *hconv = cmalloc(sizeof(LPF_HILBERT_QUAD));

 hconv -> iir_I = iir_rp_create(fdescr, is_kahan);
 hconv -> iir_Q = iir_rp_create(fdescr, is_kahan);
 hconv -> sampe_ix = 0;
 return hconv;
}

/* destroy the converter
*/
void hq_rp_destroy(LPF_HILBERT_QUAD *hconv)
{
 if(hconv)
 {
  if(hconv -> iir_I)
  {
   iir_rp_destroy(hconv -> iir_I);
   hconv -> iir_I = NULL;
  }
  if(hconv -> iir_Q)
  {
   iir_rp_destroy(hconv -> iir_Q);
   hconv -> iir_Q = NULL;
  }

  free(hconv);
  hconv = NULL;                         // :))
 }
}

/* return string list ordered according hq_rp_create_ix() indexes
*/
const TCHAR **hq_get_type_names(void)
{
 static const TCHAR *type_names[] =
 {
  _T("HB LPF Type 0 (weak, ord. 15)"),          // [IX_IIR_LOEL_TYPE0]
  _T("HB LPF Type 1 (strong, ord. 19)"),        // [IX_IIR_LOEL_TYPE1]
  _T("HB LPF Type 2 (medium, ord. 18)"),        // [IX_IIR_LOEL_TYPE2]
  _T("HB LPF Type 3, !UGLY!, ord. 19"),         // [IX_IIR_LOEL_TYPE3]
  _T("HB LPF Type 4, !UGLY!, ord. 20"),         // [IX_IIR_LOEL_TYPE4]
  _T("HB LPF Type 5, !UGLY!, ord. 20"),         // [IX_IIR_LOEL_TYPE5]
  NULL
 };

 return type_names;
}

/* process one sample
*/
void hq_rp_process(double sample, double *outI, double *outQ, LPF_HILBERT_QUAD *hconv, FP_EXCEPT_STATS *fes)
{
 // really gracefull and beauty
 switch(hconv -> sampe_ix)
 {
  case 0:       // a = in; b = 0; s.re = I; s.im = Q
   *outI =  iir_rp_process( sample, hconv -> iir_I, fes) * 2.0;
   *outQ =  iir_rp_process( 0.0   , hconv -> iir_Q, fes) * 2.0;
   break;

  case 1:       // a = 0; b = -in; s.re = -Q; s.im = I
   *outI = -iir_rp_process(-sample, hconv -> iir_Q, fes) * 2.0;
   *outQ =  iir_rp_process( 0.0   , hconv -> iir_I, fes) * 2.0;
   break;

  case 2:       // a = -in; b = 0; s.re = -I; s.im = -Q
   *outI = -iir_rp_process(-sample, hconv -> iir_I, fes) * 2.0;
   *outQ = -iir_rp_process( 0.0   , hconv -> iir_Q, fes) * 2.0;
   break;

  case 3:       // a = 0; b = in; s.re = Q; s.im = -I
   *outI =  iir_rp_process( sample, hconv -> iir_Q, fes) * 2.0;
   *outQ = -iir_rp_process( 0.0   , hconv -> iir_I, fes) * 2.0;
   break;
 }

 hconv -> sampe_ix = (hconv -> sampe_ix + 1) & 3;       // m_sample = (m_sample + 1) % 4;
}

/* reset the converter
*/
void hq_rp_reset(LPF_HILBERT_QUAD *hconv)
{
 iir_rp_reset(hconv -> iir_I);
 iir_rp_reset(hconv -> iir_Q);
 hconv -> sampe_ix = 0;
}

/* change summation algorithm
*/
void hq_rp_setsum(LPF_HILBERT_QUAD *hconv, BOOL is_kahan)
{
 iir_rp_setsum(hconv -> iir_I, is_kahan);
 iir_rp_setsum(hconv -> iir_Q, is_kahan);
}

/* the end...
*/

