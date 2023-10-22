/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      lpf_hilbert_quad.h -- all about making analitic signal via HB LPF
 *      The main idea of this non-trivial transform algorithm takan from
 *      Serge Bahurin, dsplib.ru / dsplib.org. Lots of thanks to him from
 *      Rat and Catcher Technologies.
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

#if !defined(_lpf_hilbert_quad_h_)
#define _lpf_hilbert_quad_h_

#include "compatibility/compat_gcc.h"

#include <tchar.h>

#include "hblpf.h"

#include "compatibility/compat_win32_gcc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* the real to analitic converter internals type
*/
typedef struct tagLPF_HILBERT_QUAD
{
 IIR_RAT_POLY *iir_I;                           // in-phase channel LPF
 IIR_RAT_POLY *iir_Q;                           // quadrature channel LPF
 IIR_RP_PROCESS p_iir_rp_process;               // the futction to process
 unsigned sampe_ix;                             // sample counter mod 4
} LPF_HILBERT_QUAD;

/* functions
*/
// -- create the converter by IIR_COEFF
LPF_HILBERT_QUAD *hq_rp_create(const RP_IIR_FILTER_DESCR *fdescr, const IIR_COMP_CONFIG *comp_cfg);
// -- destroy the converter
void hq_rp_destroy(LPF_HILBERT_QUAD *hconv);
// -- return string list ordered according hq_rp_create_ix() indexes
const TCHAR **hq_get_type_names(void);
// -- process one sample
void hq_rp_process(double sample, double *outI, double *outQ, LPF_HILBERT_QUAD *hconv, FP_EXCEPT_STATS *fes);
// -- reset the converter
void hq_rp_reset(LPF_HILBERT_QUAD *hconv);
// -- change IIR computation parameters
void hq_rp_setcfg(const LPF_HILBERT_QUAD *hconv, const IIR_COMP_CONFIG *comp_cfg);
// -- reset subnorm rejections counters
void hq_rp_reset_sncnt(const LPF_HILBERT_QUAD *hconv);
// -- get current summary subnorm rejecttions counter
uint64_t hq_rp_get_sncnt(const LPF_HILBERT_QUAD *hconv);

// the creation by the index (IX_IIR_LOEL_xxx)
static __inline LPF_HILBERT_QUAD *hq_rp_create_ix(unsigned index, const IIR_COMP_CONFIG *comp_cfg)
{
 return hq_rp_create(&iir_hb_lpf_const_filters[index], comp_cfg);
}


#if defined(__cplusplus)
}
#endif

#endif                                  // def _lpf_hilbert_quad_h_

/* the end...
*/

