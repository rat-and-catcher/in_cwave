/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      hblpf.h -- all about Half Band Low Pass Elliptic Filters
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

#if !defined(_hblpf_h_)
#define _hblpf_h_

#if defined(__cplusplus)
extern "C" {
#endif

#include "fp_check.h"

/* the IIR filters with predefined coefficients
*/
// IIR filter types:
#define IIR_FT_MASK     (0x0000000F)            /* the mask of the filter type */
#define IIR_FT_LOWPASS  (0)                     /* low pass */
#define IIR_FT_HIGHPASS (1)                     /* high pass */
#define IIR_FT_BANDPASS (2)                     /* band pass */
#define IIR_FT_NOTCH    (3)                     /* notch filter */

// IIR filter approximation types:
#define IIR_FA_MASK     (0xFFFFFFF0)            /* the mask of the approximation type */
#define IIR_FA_SHIFT    (4)                     /* shift to approximation type */
#define IIR_FA_BUTTER   (1 << IIR_FA_SHIFT)     /* Butterwort */
#define IIR_FA_CHEB1    (2 << IIR_FA_SHIFT)     /* 1-st kind Chebyshev */
#define IIR_FA_CHEB2    (3 << IIR_FA_SHIFT)     /* 2-nd kind Chebyshev */
#define IIR_FA_ELLIPTIC (4 << IIR_FA_SHIFT)     /* elliptic filter */

// H(Z) function descriptor
typedef struct tag_IIR_COEFF
{
 double *b;                             // numerator [n_b + 1] (polynomal of order n_b)
 double *a;                             // denominator [n_a + 1] (polynomal of order n_a)
 int n_b;                               // order of numerator / ## elements in b[] - 1
 int n_a;                               // order of denominator / ## elements in a[] - 1
} IIR_COEFF;

// the IIR filter description
typedef struct tag_IIR_DESCR
{
 // filter design characteristics
 double irreg;                          // ripple in pass band (dB)
 double atten;                          // attenuation in stop band (dB)
 double fpass_low;                      // low frequency for pass band [0..1]
 double fstop_low;                      // low frequency for stop band [0..1]
 double fpass_high;                     // high frequency for pass band [0..1] (band/notch only)
 double fstop_high;                     // high frequency for stop band [0..1] (band/notch only)
 unsigned type_approx;                  // filter type+approximation IIR_FT_xxx | IIR_FA_xxx
 int order;                             // filter order
 // the filter for processing
 IIR_COEFF iir_coefficients;            // IIR coefficients
} IIR_DESCR;


/* the static table of our "canonical" HB LPFs
*/
// variants:
// Low pass: 0.495000/0.505000; ripple 1.500000 dB; rejection 100.000000 dB -- order 15
#define IX_IIR_LOEL_TYPE0               (0)
// Low pass: 0.499000/0.502000; ripple 1.800000 dB; rejection 100.000000 dB -- order 19
#define IX_IIR_LOEL_TYPE1               (1)
// Low pass: 0.499000/0.501000; ripple 2.000000 dB; rejection 90.000000 dB -- order 18
#define IX_IIR_LOEL_TYPE2               (2)
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
extern const IIR_DESCR iir_hb_lpf_const_filters[];

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
};

/* functions
*/
// -- create the filter by IIR_COEFF
IIR_RAT_POLY *iir_rp_create(const IIR_COEFF *coeffs, BOOL is_kahan);
// -- destroy the filter
void iir_rp_destroy(IIR_RAT_POLY *filter);
// -- process one sample -- canonical form, min. computations, BUT NOT SO GOOD FOR ROUNDING ERRORS
double iir_rp_process_baseline(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes);
// -- process one sample -- canonical form / Kahan summations
double iir_rp_process_kahan(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes);
// -- reset the filter
void iir_rp_reset(IIR_RAT_POLY *filter);
// -- change summation algorithm
void iir_rp_setsum(IIR_RAT_POLY *filter, BOOL is_kahan);

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

