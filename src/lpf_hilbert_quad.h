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
 unsigned sampe_ix;                             // sample counter mod 4
} LPF_HILBERT_QUAD;

/* the internal descriptor for IIR HB LPF filter (and, probably, corresponded stuff)
*/
typedef struct tagLPF_HILBERT_FILTER
{
 const RP_IIR_FILTER_DESCR *half_band;          // the "main" low-pass half-band filter
 const TCHAR *hilb_descr;                       // description string
} LPF_HILBERT_FILTER;

/* possible filters indexes (correspond to IX_IIR_LOEL_xxx)
*/
// Low pass: 0.495000/0.505000; ripple 1.500000 dB; rejection 100.000000 dB -- order 15
#define IX_LPF_HILB_TYPE0               (0)
// Low pass: 0.499000/0.502000; ripple 1.800000 dB; rejection 100.000000 dB -- order 19
#define IX_LPF_HILB_TYPE1               (1)
// Low pass: 0.499000/0.501000; ripple 2.000000 dB; rejection 90.000000 dB -- order 18
#define IX_LPF_HILB_TYPE2               (2)
/* not so "canonical"
*/
// Low pass: 0.498200/0.500000; ripple 2.000000 dB; rejection 96.000000 dB -- order 19
#define IX_LPF_HILB_TYPE3               (3)
// Low pass: 0.498200/0.500000; ripple 2.000000 dB; rejection 100.000000 dB -- order 20
#define IX_LPF_HILB_TYPE4               (4)
// Low pass: 0.498500/0.500000; ripple 2.000000 dB; rejection 100.000000 dB -- order 20
#define IX_LPF_HILB_TYPE5               (5)
// [...to be continued...]
#define IX_LPF_HILB_TYPEMAX             (IX_LPF_HILB_TYPE5)

// the default filter type
#define IX_LPF_HILB_DEF                 (IX_LPF_HILB_TYPE1)

// the number of filters
#define NM_LPF_HILB                     (6)

/* functions
*/
// the creation by the index (IX_LPF_HILB_xxx)
LPF_HILBERT_QUAD *hq_rp_create_ix(unsigned index, const IIR_COMP_CONFIG *comp_cfg);
// -- create the converter by LPF_HILBERT_FILTER
LPF_HILBERT_QUAD *hq_rp_create(const LPF_HILBERT_FILTER *fdescr, const IIR_COMP_CONFIG *comp_cfg);
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


#if defined(__cplusplus)
}
#endif

#endif                                  // def _lpf_hilbert_quad_h_

/* the end...
*/

