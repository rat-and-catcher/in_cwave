/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      hblpf.h -- all about Half Band Low Pass Elliptic Filters
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

#include <stdlib.h>

#include "cmalloc.h"
#include "hblpf.h"

/* THE PRECALCULATED elliptic HB LPFs
 * --- ------------- -------- -- ----
 * this filters was calculated by climed prototypes (designs) using third party tool - DSP.DLL,
 * obtained from the site dsplib.ru, where it's lived some time (w/o sources, only API +
 * win32 binary) and was available fo free. DSP.DLL was written by Serge Bahurin, dsplib.ru.
 * Unfortunally, now this binary unavailable - it was removed from the site by author as "unsupported".
 * But the site still alive, and even has an English version at dsplib.org. We take many great
 * ideas from Serge Bahurin's site materials, especially from his papers -- many thanks for him
 * from Rat and Catcher Technologies.
 *
 * NOTE: we can't get stable elliptic filters of order greater 19 for our needs (whith
 * extremly small transition width + high rejection (>100dB) and acceptable irregularity
 * in pass band. May be latter...
 */

// Low pass: 0.495000/0.505000; irregularity 1.500000 dB; rejection 100.000000 dB -- order 15
static double IIR_LOEL_TYPE0_B[] =              // 16 coefficients
{
   2.149098742015852700000E-003,    // _B[ 0]
   9.123050273140889000000E-003,    // _B[ 1]
   2.966925832301847700000E-002,    // _B[ 2]
   6.811157814758100900000E-002,    // _B[ 3]
   1.295620086221753200000E-001,    // _B[ 4]
   2.031184600983565500000E-001,    // _B[ 5]
   2.729801687594835000000E-001,    // _B[ 6]
   3.150682970206441500000E-001,    // _B[ 7]
   3.150682970206441500000E-001,    // _B[ 8]
   2.729801687594835000000E-001,    // _B[ 9]
   2.031184600983565500000E-001,    // _B[10]
   1.295620086221753200000E-001,    // _B[11]
   6.811157814758100900000E-002,    // _B[12]
   2.966925832301847700000E-002,    // _B[13]
   9.123050273140889000000E-003,    // _B[14]
   2.149098742015852700000E-003     // _B[15]
};
static double IIR_LOEL_TYPE0_A[] =              // 16 coefficients
{
   1.000000000000000000000E+000,    // _A[ 0]
  -3.467830974494252600000E+000,    // _A[ 1]
   1.081904751617984900000E+001,    // _A[ 2]
  -2.246144873583340000000E+001,    // _A[ 3]
   4.050865688803637700000E+001,    // _A[ 4]
  -5.903881941961940100000E+001,    // _A[ 5]
   7.505347218893619800000E+001,    // _A[ 6]
  -8.128749955628022400000E+001,    // _A[ 7]
   7.680933336059548800000E+001,    // _A[ 8]
  -6.254671597905413400000E+001,    // _A[ 9]
   4.389178018084239600000E+001,    // _A[10]
  -2.618201480754600400000E+001,    // _A[11]
   1.293971250302366500000E+001,    // _A[12]
  -5.174558352697588700000E+000,    // _A[13]
   1.493417932428228800000E+000,    // _A[14]
  -2.969689045443764700000E-001     // _A[15]
};

// Low pass: 0.499000/0.502000; irregularity 1.800000 dB; rejection 100.000000 dB -- order 19
static double IIR_LOEL_TYPE1_B[] =              // 20 coefficients
{
   2.121647248355896600000E-003,    // _B[ 0]
   8.943574251885097100000E-003,    // _B[ 1]
   3.331432108639852100000E-002,    // _B[ 2]
   8.447725518252464900000E-002,    // _B[ 3]
   1.868628869947921400000E-001,    // _B[ 4]
   3.404188235370274600000E-001,    // _B[ 5]
   5.486900620199420800000E-001,    // _B[ 6]
   7.705996064146916100000E-001,    // _B[ 7]
   9.668599396922500400000E-001,    // _B[ 8]
   1.079554554639516900000E+000,    // _B[ 9]
   1.079554554639516900000E+000,    // _B[10]
   9.668599396922500400000E-001,    // _B[11]
   7.705996064146916100000E-001,    // _B[12]
   5.486900620199420800000E-001,    // _B[13]
   3.404188235370274600000E-001,    // _B[14]
   1.868628869947921400000E-001,    // _B[15]
   8.447725518252464900000E-002,    // _B[16]
   3.331432108639852100000E-002,    // _B[17]
   8.943574251885097100000E-003,    // _B[18]
   2.121647248355896600000E-003     // _B[19]
};
static double IIR_LOEL_TYPE1_A[] =              // 20 coefficients
{
   1.000000000000000000000E+000,    // _A[ 0]
  -3.485871872722654100000E+000,    // _A[ 1]
   1.289714153117239400000E+001,    // _A[ 2]
  -2.967820786511090200000E+001,    // _A[ 3]
   6.385316363510077100000E+001,    // _A[ 4]
  -1.089445729540421700000E+002,    // _A[ 5]
   1.695872514974658700000E+002,    // _A[ 6]
  -2.260335319941291600000E+002,    // _A[ 7]
   2.731425037288854600000E+002,    // _A[ 8]
  -2.910703469060932200000E+002,    // _A[ 9]
   2.799496715148561100000E+002,    // _A[10]
  -2.396301663729838100000E+002,    // _A[11]
   1.834970973449506700000E+002,    // _A[12]
  -1.245951047569147400000E+002,    // _A[13]
   7.420602323458477700000E+001,    // _A[14]
  -3.854241469502378700000E+001,    // _A[15]
   1.673541768003197900000E+001,    // _A[16]
  -6.110443227816650100000E+000,    // _A[17]
   1.588094610260089000000E+000,    // _A[18]
  -3.220187903362500300000E-001     // _A[19]
};

// Low pass: 0.499000/0.501000; irregularity 2.000000 dB; rejection 90.000000 dB -- order 18
static double IIR_LOEL_TYPE2_B[] =              // 19 coefficients
{
   3.735778782125440700000E-003,    // _B[ 0]
   1.424898058026445900000E-002,    // _B[ 1]
   5.171182409414783900000E-002,    // _B[ 2]
   1.233954030719509700000E-001,    // _B[ 3]
   2.632123746956572300000E-001,    // _B[ 4]
   4.555940689115121600000E-001,    // _B[ 5]
   7.041779414153245300000E-001,    // _B[ 6]
   9.398014775260661300000E-001,    // _B[ 7]
   1.124832711319702500000E+000,    // _B[ 8]
   1.186707662214442600000E+000,    // _B[ 9]
   1.124832711319702500000E+000,    // _B[10]
   9.398014775260661300000E-001,    // _B[11]
   7.041779414153245300000E-001,    // _B[12]
   4.555940689115121600000E-001,    // _B[13]
   2.632123746956572300000E-001,    // _B[14]
   1.233954030719509700000E-001,    // _B[15]
   5.171182409414783900000E-002,    // _B[16]
   1.424898058026445900000E-002,    // _B[17]
   3.735778782125440700000E-003     // _B[18]
};
static double IIR_LOEL_TYPE2_A[] =              // 19 coefficients
{
   1.000000000000000000000E+000,    // _A[ 0]
  -3.145881662471560900000E+000,    // _A[ 1]
   1.151284949465824200000E+001,    // _A[ 2]
  -2.478638672818869500000E+001,    // _A[ 3]
   5.192110878119253700000E+001,    // _A[ 4]
  -8.386813901462475900000E+001,    // _A[ 5]
   1.258795437014948400000E+002,    // _A[ 6]
  -1.591761538012566700000E+002,    // _A[ 7]
   1.839070447875357400000E+002,    // _A[ 8]
  -1.851817906746923400000E+002,    // _A[ 9]
   1.686444470786563300000E+002,    // _A[10]
  -1.349725102133759500000E+002,    // _A[11]
   9.657897262088118900000E+001,    // _A[12]
  -5.999568328632901200000E+001,    // _A[13]
   3.273105996696668300000E+001,    // _A[14]
  -1.479141017962676900000E+001,    // _A[15]
   5.700063007021833000000E+000,    // _A[16]
  -1.534966283782960700000E+000,    // _A[17]
   3.392889541592242800000E-001     // _A[18]
};

// the static table of filters
const IIR_DESCR iir_hb_lpf_const_filters[] =
{
 {
  1.500000,             // irregularity in pass band (dB)
  100.000000,           // attenuation
  0.495000,             // low frequency for pass band [0..1]
  0.505000,             // low frequency for stop band [0..1]
  0.000000,             // high frequency for pass band (band/notch)
  0.000000,             // high frequency for stop band (band/notch)
  IIR_FT_LOWPASS | IIR_FA_ELLIPTIC,
  15,                   // the order
  {                     // The Filter:
   IIR_LOEL_TYPE0_B,
   IIR_LOEL_TYPE0_A,
   sizeof(IIR_LOEL_TYPE0_B) / sizeof(double) - 1,       // sizeof() not a constant expr. in C, we know
   sizeof(IIR_LOEL_TYPE0_A) / sizeof(double) - 1        // sizeof() not a constant expr. in C, we know
  }
 },

 {
  1.800000,             // irregularity in pass band (dB)
  100.000000,           // attenuation
  0.499000,             // low frequency for pass band [0..1]
  0.502000,             // low frequency for stop band [0..1]
  0.000000,             // high frequency for pass band (band/notch)
  0.000000,             // high frequency for stop band (band/notch)
  IIR_FT_LOWPASS | IIR_FA_ELLIPTIC,
  19,                   // the order
  {                     // The Filter:
   IIR_LOEL_TYPE1_B,
   IIR_LOEL_TYPE1_A,
   sizeof(IIR_LOEL_TYPE1_B) / sizeof(double) - 1,       // sizeof() not a constant expr. in C, we know
   sizeof(IIR_LOEL_TYPE1_A) / sizeof(double) - 1        // sizeof() not a constant expr. in C, we know
  }
 },

 {
  2.000000,             // irregularity in pass band (dB)
  90.000000,            // attenuation
  0.499000,             // low frequency for pass band [0..1]
  0.501000,             // low frequency for stop band [0..1]
  0.000000,             // high frequency for pass band (band/notch)
  0.000000,             // high frequency for stop band (band/notch)
  IIR_FT_LOWPASS | IIR_FA_ELLIPTIC,
  18,                   // the order
  {                     // The Filter:
   IIR_LOEL_TYPE2_B,
   IIR_LOEL_TYPE2_A,
   sizeof(IIR_LOEL_TYPE2_B) / sizeof(double) - 1,       // sizeof() not a constant expr. in C, we know
   sizeof(IIR_LOEL_TYPE2_A) / sizeof(double) - 1        // sizeof() not a constant expr. in C, we know
  }
 }

// [...to be continued...]
};

/* filter computation implementation
 * ------ ----------- --------------
 */
/* create the filter by IIR_COEFF
*/
IIR_RAT_POLY *iir_rp_create(const IIR_COEFF *coeffs)
{
 IIR_RAT_POLY *filter = cmalloc(sizeof(IIR_RAT_POLY));
 int i;


 filter -> pc = filter -> pd = filter -> pz = NULL;
 filter -> nord = coeffs -> n_a > coeffs -> n_b? coeffs -> n_a : coeffs -> n_b;

 // we suppose, that the new operator always successful
 filter -> pc = cmalloc(filter -> nord * sizeof(double));
 filter -> pd = cmalloc(filter -> nord * sizeof(double));
 filter -> pz = cmalloc(filter -> nord * sizeof(double));

 filter -> d0 = coeffs -> b[0] / coeffs -> a[0];
 for(i = 0; i < filter -> nord; ++i)
 {
  int j = i + 1;
  filter -> pc[i] = j <= coeffs -> n_a? -coeffs -> a[j] / coeffs -> a[0] : 0.0;
  filter -> pd[i] = j <= coeffs -> n_b?  coeffs -> b[j] / coeffs -> a[0] : 0.0;
 }

 iir_rp_reset(filter);
 return filter;
}

/* destroy the filter
*/
void iir_rp_destroy(IIR_RAT_POLY *filter)
{
 if(filter)
 {
  if(filter -> pz)
  {
   free(filter -> pz);
   filter -> pz = NULL;
  }
  if(filter -> pd)
  {
   free(filter -> pd);
   filter -> pd = NULL;
  }
  if(filter -> pc)
  {
   free(filter -> pc);
   filter -> pc = NULL;
  }

  free(filter);
  filter = NULL;                        // :))
 }
}

/* process one sample -- canonical form, min. computations, BUT NOT SO GOOD FOR ROUNDING ERRORS
*/
double iir_rp_process(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes)
{
#define FES         (fes)

 double sum_i = sample;
 double sum_o = 0.0;
 int k = filter -> ix;
 int i;

 if(!fes -> is_enable)
 {                                              // == WITHOUT FP CHECKS ==
  for(i = 0; i < filter -> nord; ++i)
  {
   if(0 == k)
    k = filter -> nord;
   --k;

   sum_i += filter -> pz[k] * filter -> pc[i];
   sum_o += filter -> pz[k] * filter -> pd[i];
  }

  filter -> pz[filter -> ix] = sum_i;
  if(++(filter -> ix) >= filter -> nord)        // m_ix = (++m_ix) % m_nord; -- it's BAD for iNTEL
   filter -> ix = 0;

  return sum_i * filter -> d0 + sum_o;
 }
 else
 {                                              // == WITH FP CHECKS ==
  for(i = 0; i < filter -> nord; ++i)
  {
   if(0 == k)
    k = filter -> nord;
   --k;

   sum_i = FC(sum_i + FC(filter -> pz[k] * filter -> pc[i]));
   sum_o = FC(sum_o + FC(filter -> pz[k] * filter -> pd[i]));
  }

  filter -> pz[filter -> ix] = sum_i;
  if(++(filter -> ix) >= filter -> nord)        // m_ix = (++m_ix) % m_nord; -- it's BAD for iNTEL
   filter -> ix = 0;

  return FC(FC(sum_i * filter -> d0) + sum_o);
 }

#undef FES
}

/* reset the filter
*/
void iir_rp_reset(IIR_RAT_POLY *filter)
{
 int i;

 for(i = 0; i < filter -> nord; ++i)
  filter -> pz[i] = 0.0;

 filter -> ix = 0;
}


/* the end...
*/

