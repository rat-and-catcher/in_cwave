/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      hblpf.h -- all about Half Band Low Pass Elliptic Filters
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

#if !defined(_hblpf_h_)
#define _hblpf_h_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

#include "fp_check.h"

/* the IIR filters with predefined coefficients
*/
// -- gen-iir (DSPL, www.dsplib.org) filter descriptor
// filter types:
#define RP_IIR_TYPE_SHIFT       (0)
#define RP_IIR_TYPE_MASK        (0xFF << RP_IIR_TYPE_SHIFT)
#define RP_IIR_LPF              (0 << RP_IIR_TYPE_SHIFT)
#define RP_IIR_HPF              (1 << RP_IIR_TYPE_SHIFT)
#define RP_IIR_BPF              (2 << RP_IIR_TYPE_SHIFT)
#define RP_IIR_BSF              (3 << RP_IIR_TYPE_SHIFT)
// approximation types:
#define RP_IIR_APPROX_SHIFT     (8)
#define RP_IIR_APPROX_MASK      (0xFF << RP_IIR_APPROX_SHIFT)
#define RP_IIR_BUTTER           (0 << RP_IIR_APPROX_SHIFT)
#define RP_IIR_CHEBY1           (1 << RP_IIR_APPROX_SHIFT)
#define RP_IIR_CHEBY2           (2 << RP_IIR_APPROX_SHIFT)
#define RP_IIR_ELLIP            (3 << RP_IIR_APPROX_SHIFT)

// the filter (design + coefficients):
typedef struct s_rp_iir_filter_descr
{
  unsigned flags;               // RP_IIR_LPF-or-so | RP_IIR_BUTTER-or-so
  int      order;
  double   suppression;
  double   rillpe;
  double   left_cutoff;
  double   right_cutoff;
  double   transition_width;
  const uint64_t *vec_b;        // order + 1 values
  const uint64_t *vec_a;        // order + 1 values
} RP_IIR_FILTER_DESCR;


/* the static table of our "canonical" HB LPFs
*/
// variants:
// Low pass: 0.495000/0.505000; ripple 1.500000 dB; rejection 100.000000 dB -- order 15
#define IX_IIR_LOEL_TYPE0               (0)
// Low pass: 0.499000/0.502000; ripple 1.800000 dB; rejection 100.000000 dB -- order 19
#define IX_IIR_LOEL_TYPE1               (1)
// Low pass: 0.499000/0.501000; ripple 2.000000 dB; rejection 90.000000 dB -- order 18
#define IX_IIR_LOEL_TYPE2               (2)
/* not so "canonical"
*/
// Low pass: 0.498200/0.500000; ripple 2.000000 dB; rejection 96.000000 dB -- order 19
#define IX_IIR_LOEL_TYPE3               (3)
// Low pass: 0.498200/0.500000; ripple 2.000000 dB; rejection 100.000000 dB -- order 20
#define IX_IIR_LOEL_TYPE4               (4)
// Low pass: 0.498500/0.500000; ripple 2.000000 dB; rejection 100.000000 dB -- order 20
#define IX_IIR_LOEL_TYPE5               (5)
// [...to be continued...]
#define IX_IIR_LOEL_TYPEMAX             (IX_IIR_LOEL_TYPE5)

// the default filter type
#define IX_IIR_LOEL_DEF                 (IX_IIR_LOEL_TYPE1)

// the number of filters
#define NM_IIR_LOEL                     (6)

// the static table of precalculated filters
extern const RP_IIR_FILTER_DESCR iir_hb_lpf_const_filters[];

/* IIR filter implementation
*/
// the filter internals type -- forward reference
typedef struct tagIIR_RAT_POLY IIR_RAT_POLY;

// the filter one sample process funstion
typedef double (*IIR_RP_PROCESS)(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes);

// the filter internals type
struct tagIIR_RAT_POLY
{
 double *pc;                // "loop-back" coefficients of canonical form [1..max(n_a,n_b)]
 double *pd;                // "direct" coefficients of canonical form [1..max(n_a,n_b)]
 double *pz;                // "delay line" of canonical form
 double d0;                 // 0-st "direct" coefficient of canonical form
 int nord;                  // the order of filter
 int ix;                    // insert index for "delay line"
 IIR_RP_PROCESS process;    // one sample process function
 BOOL is_subnorm_reject;    // should we suppress low values in "delay line" of IIR state
 double subnorm_thr;        // subnorm threshold
 uint64_t subnorm_cnt;      // counter of subnorm reject actions
};

// the additional filter configuration staff
typedef struct tagIIR_COMP_CONFIG
{
 BOOL is_kahan;             // use Kahan summation algorithm
 BOOL is_subnorm_reject;    // should we suppress low values in "delay line" of IIR state
 double subnorm_thr;        // subnorm reject threshold
} IIR_COMP_CONFIG;

// threshold bound values
#define SBN_THR_MAX         (1.0E-40)           /* slightly greater than PDP11 FP range */
#define SBN_THR_MIN         (1.0E-300)          /* slightly bigger than x86 DBL_MIN */
#define SBN_THR_DEF         (1.0E-150)          /* looks rather good (or not, sorry) */

/* functions -- !!ALL ARE THREAD UNSAFE!! !!ALL DON'T CHECK CONTRACT!!
*/
// -- create the filter by IIR_COEFF
IIR_RAT_POLY *iir_rp_create(const RP_IIR_FILTER_DESCR *fdescr, const IIR_COMP_CONFIG *comp_cfg);
// -- destroy the filter
void iir_rp_destroy(IIR_RAT_POLY *filter);
// -- process one sample -- canonical form, min. computations, BUT NOT SO GOOD FOR ROUNDING ERRORS
double iir_rp_process_baseline(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes);
// -- process one sample -- canonical form / Kahan summations
double iir_rp_process_kahan(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes);
// -- reset the filter
void iir_rp_reset(IIR_RAT_POLY *filter);
// -- change computation rules
void iir_rp_setcfg(IIR_RAT_POLY *filter, const IIR_COMP_CONFIG *comp_cfg);
// -- reset subnorm rejecttions counter
void iir_rp_reset_sncnt(IIR_RAT_POLY *filter);
// -- get current subnorm rejecttions counter
uint64_t iir_rp_get_sncnt(IIR_RAT_POLY *filter);

// -- process one sample -- generic frontend
static __inline double iir_rp_process(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes)
{
 return (*(filter -> process))(sample, filter, fes);
}

#if defined(__cplusplus)
}
#endif

#endif                                  // def _hblpf_h_

/* the end...
*/

