/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      sound_render.c -- all about convertig double to the integer sound samples
 *
 * Copyright (c) 2010-2025, Rat and Catcher Technologies
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

#include "sound_render.h"

#include "atomic.h"
#include "cmalloc.h"

/* NOTE::We work with doubles, which normalized (or must to be normalized)
 * to [-32768.0..32767.0] (really [-32768.0..32768.0]).
 * For good or for evil -- it's our solution.
 */

/* NOTE ABOUT NOISE SHAPER IMPLEMENTATION:: the '.target_sample_rate' field in NS_SHAPER_DSC
 * not used in current implementation; the only user has to properly select the shaper for particular source
 * sample rate. If user's choise will be improper, the result will be improper (up to unpredictable).
 */

/* constant(s)
*/
// sqrt(6) == Sigma[TPDF(-1.0..+1.0)] -- _our_ noise "equivalent" for 1 to +-LSBit @ TPDF noise
#define SQRT6           (2.4494897427831780981972840747059)
// sqrt(2): Sigma[RPDF(-1.0..+1.0)]{1/sqrt(3)} == sqrt(2)*Sigma[TPDF(-1.0..+1.0)]{1/sqrt(6)}
#define SQRT2           (1.4142135623730950488016887242097)

/* forwards
 * --------
 */
/* the "empty" noise shaping filter
*/
static double ns_empty(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes);
/* the FIR noise shaping filter
*/
static double ns_fir(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes);
/* the IIR noise shaping filter
*/
static double ns_iir(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes);

/* some noise shaping filters
 * ---- ----- ------- -------
 * (we don't intended to collect all of the known noise shapers, but there is all of the SoX filters)
 *
 * so, look::there are lots of really magic numbers!!
 */
// 44100 Hz -- F-weighted; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_fwght_44100[] =
{
 2.412, -3.370, 3.937, -4.174, 3.353, -2.205, 1.281, -0.569, 0.0847
};
// 44100/48000 Hz -- modified E-weighted; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_mewght_44100[] =
{
 1.662, -1.263, 0.4827, -0.2913, 0.1268, -0.1124, 0.03252, -0.01265, -0.03524
};
// 44100/48000 Hz -- improved E-weighted; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_iewght_44100[] =
{
 2.847, -4.685, 6.214, -7.184, 6.639, -5.032, 3.263, -1.632, 0.4191
};
// 44100 Hz -- Lipshitz; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_lipshitz_44100[] =
{
 2.033, -2.165, 1.959, -1.590, 0.6149
};
// 48000 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_48000[] =
{
  2.8720729351043701172,  -5.0413231849670410156,   6.2442994117736816406,
 -5.8483986854553222656,   3.7067542076110839844,  -1.0495119094848632812,
 -1.1830236911773681641,   2.1126792430877685547,  -1.9094531536102294922,
  0.99913084506988525391, -0.17090806365013122559, -0.32615602016448974609,
  0.39127644896507263184, -0.26876461505889892578,  0.097676105797290802002,
 -0.023473845794796943665
};
// 44100 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_44100[] =
{
  2.6773197650909423828,  -4.8308925628662109375,   6.570110321044921875,
 -7.4572014808654785156,   6.7263274192810058594,  -4.8481650352478027344,
  2.0412089824676513672,   0.7006359100341796875,  -2.9537565708160400391,
  4.0800385475158691406,  -4.1845216751098632812,   3.3311812877655029297,
 -2.1179926395416259766,   0.879302978515625,      -0.031759146600961685181,
 -0.42382788658142089844,  0.47882103919982910156, -0.35490813851356506348,
  0.17496839165687561035, -0.060908168554306030273
};
// 38000 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_38000[] =
{
  1.6335992813110351562,   -2.2615492343902587891,   2.4077029228210449219,
 -2.6341717243194580078,    2.1440362930297851562,  -1.8153258562088012695,
  1.0816224813461303711,   -0.70302653312683105469,  0.15991993248462677002,
  0.041549518704414367676, -0.29416576027870178223,  0.2518316805362701416,
 -0.27766478061676025391,   0.15785403549671173096, -0.10165894031524658203,
  0.016833892092108726501
};
// 32000 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_32000[] =
{ /* dmaker 32000: bestmax=4.99659 (inverted) */
  0.82118552923202515,  -1.0063692331314087,   0.62341964244842529,
 -1.0447187423706055,    0.64532512426376343, -0.87615132331848145,
  0.52219754457473755,  -0.67434263229370117,  0.44954317808151245,
 -0.52557498216629028,   0.34567299485206604, -0.39618203043937683,
  0.26791760325431824,  -0.28936097025871277,  0.1883765310049057,
 -0.19097308814525604,   0.10431359708309174, -0.10633844882249832,
  0.046832218766212463, -0.039653312414884567
};
// 22050 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_22050[] =
{ /* dmaker 22050: bestmax=5.77762 (inverted) */
  0.056581053882837296,  -0.56956905126571655,  -0.40727734565734863,
 -0.33870288729667664,   -0.29810553789138794,  -0.19039161503314972,
 -0.16510021686553955,   -0.13468159735202789,  -0.096633769571781158,
 -0.081049129366874695,  -0.064953058958053589, -0.054459091275930405,
 -0.043378707021474838,  -0.03660014271736145,  -0.026256965473294258,
 -0.018786206841468811,  -0.013387725688517094, -0.0090983230620622635,
 -0.0026585909072309732, -0.00042083300650119781,
};
// 16000 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_16000[] =
{ /* dmaker 16000: bestmax=5.97128 (inverted) */
 -0.37251132726669312,  -0.81423574686050415,  -0.55010956525802612,
 -0.47405767440795898,  -0.32624706625938416,  -0.3161766529083252,
 -0.2286367267370224,   -0.22916607558727264,  -0.19565616548061371,
 -0.18160104751586914,  -0.15423151850700378,  -0.14104481041431427,
 -0.11844276636838913,  -0.097583092749118805, -0.076493598520755768,
 -0.068106919527053833, -0.041881654411554337, -0.036922425031661987,
 -0.019364040344953537, -0.014994367957115173
};
// 11025 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_11025[] =
{ /* dmaker 11025: bestmax=5.9406 (inverted) */
 -0.9264228343963623,   -0.98695987462997437,  -0.631156325340271,
 -0.51966935396194458,  -0.39738872647285461,  -0.35679301619529724,
 -0.29720726609230042,  -0.26310476660728455,  -0.21719355881214142,
 -0.18561814725399017,  -0.15404847264289856,  -0.12687471508979797,
 -0.10339745879173279,  -0.083688631653785706, -0.05875682458281517,
 -0.046893671154975891, -0.027950936928391457, -0.020740609616041183,
 -0.009366452693939209, -0.0060260160826146603
};
// 8000 Hz -- Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_shibata_8000[] =
{ /* dmaker 8000: bestmax=5.56234 (inverted) */
 -1.202863335609436,    -0.94103097915649414,  -0.67878556251525879,
 -0.57650017738342285,  -0.50004476308822632,  -0.44349345564842224,
 -0.37833768129348755,  -0.34028723835945129,  -0.29413089156150818,
 -0.24994957447052002,  -0.21715600788593292,  -0.18792112171649933,
 -0.15268312394618988,  -0.12135542929172516,  -0.099610626697540283,
 -0.075273610651493073, -0.048787496984004974, -0.042586319148540497,
 -0.028991291299462318, -0.011869125068187714
};
// 48000 Hz -- Low Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_low_shibata_48000[] =
{
  2.3925774097442626953,   -3.4350297451019287109,   3.1853709220886230469,
 -1.8117271661758422852,   -0.20124770700931549072,  1.4759907722473144531,
 -1.7210904359817504883,    0.97746700048446655273, -0.13790138065814971924,
 -0.38185903429985046387,   0.27421241998672485352,  0.066584214568138122559,
 -0.35223302245140075684,   0.37672343850135803223, -0.23964276909828186035,
  0.068674825131893157959
};
// 44100 Hz -- Low Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_low_shibata_44100[] =
{
  2.0833916664123535156,  -3.0418450832366943359,    3.2047898769378662109,
 -2.7571926116943359375,   1.4978630542755126953,   -0.3427594602108001709,
 -0.71733748912811279297,  1.0737057924270629883,   -1.0225815773010253906,
  0.56649994850158691406, -0.20968692004680633545,  -0.065378531813621520996,
  0.10322438180446624756, -0.067442022264003753662, -0.00495197344571352005
};
// 44100 Hz -- High Shibata; ns_fir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_fir_high_shibata_44100[] =
{
  3.0259189605712890625,  -6.0268716812133789062,    9.195003509521484375,
 -11.824929237365722656,   12.767142295837402344,   -11.917946815490722656,
  9.1739168167114257812,  -5.3712320327758789062,    1.1393624544143676758,
  2.4484779834747314453,  -4.9719839096069335938,    6.0392003059387207031,
 -5.9359521865844726562,   4.903278350830078125,    -3.5527443885803222656,
  2.1909697055816650391,  -1.1672389507293701172,    0.4903914332389831543,
 -0.16519790887832641602,  0.023217858746647834778
}; 
// 44100 Hz -- Gesemann; ns_iir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_iir_gesemann_44100[] =
{
  2.2061, -0.4706, -0.2534, -0.6214, 1.0587, 0.0676, -0.6054, -0.2738
};
// 48000 Hz -- Gesemann; ns_iir() to calculate.
// Origin: SoX - Sound eXchange, http://sox.sourceforge.net/
static const double ns_iir_gesemann_48000[] =
{
  2.2374, -0.7339, -0.1251, -0.6033, 0.903, 0.0116, -0.5853, -0.2571
};

/* the available noise shaping descriptors
 * --- --------- ----- ------- -----------
 */
static const NS_SHAPER_DSC shaper_dscs[] =
{
// no noise shaping [SND_NSHAPE_FLAT]
 {
   &ns_empty,
   NULL,
   0,
   0, /* any sampling frequency */
   _T("Flat (no noise shaping)")
},
// 44100 Hz -- F-weighted; ns_fir() to calculate. [SND_NSHAPE_FW44]
 {
   &ns_fir,
   ns_fir_fwght_44100,
   sizeof(ns_fir_fwght_44100) / sizeof(double),
   44100,
   _T("[44K]F-weighted")
 },
// 44100/48000 Hz -- modified E-weighted; ns_fir() to calculate. [SND_NSHAPE_MEW44]
 {
   &ns_fir,
   ns_fir_mewght_44100,
   sizeof(ns_fir_mewght_44100) / sizeof(double),
   44100,
   _T("[44-48K]Modified E-wtd")
 },
// 44100/48000 Hz -- improved E-weighted; ns_fir() to calculate. [SND_NSHAPE_IEW44]
 {
   &ns_fir,
   ns_fir_iewght_44100,
   sizeof(ns_fir_iewght_44100) / sizeof(double),
   44100,
   _T("[44-48K]Improved E-wtd")
 },
// 44100 Hz -- Lipshitz; ns_fir() to calculate. [SND_NSHAPE_LIPSH44]
 {
   &ns_fir,
   ns_fir_lipshitz_44100,
   sizeof(ns_fir_lipshitz_44100) / sizeof(double),
   44100,
   _T("[44K]Lipshitz")
 },
// 48000 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB48]
 {
   &ns_fir,
   ns_fir_shibata_48000,
   sizeof(ns_fir_shibata_48000) / sizeof(double),
   48000,
   _T("[48K]Shibata")
 },
// 44100 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB44]
 {
   &ns_fir,
   ns_fir_shibata_44100,
   sizeof(ns_fir_shibata_44100) / sizeof(double),
   44100,
   _T("[44K]Shibata")
 },
// 38000 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB38]
 {
   &ns_fir,
   ns_fir_shibata_38000,
   sizeof(ns_fir_shibata_38000) / sizeof(double),
   38000,
   _T("[38K]Shibata")
 },
// 32000 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB32]
 {
   &ns_fir,
   ns_fir_shibata_32000,
   sizeof(ns_fir_shibata_32000) / sizeof(double),
   32000,
   _T("[32K]Shibata")
 },
// 22050 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB22]
 {
   &ns_fir,
   ns_fir_shibata_22050,
   sizeof(ns_fir_shibata_22050) / sizeof(double),
   22050,
   _T("[22K]Shibata")
 },
// 16000 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB16]
 {
   &ns_fir,
   ns_fir_shibata_16000,
   sizeof(ns_fir_shibata_16000) / sizeof(double),
   16000,
   _T("[16K]Shibata")
 },
// 11025 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB11]
 {
   &ns_fir,
   ns_fir_shibata_11025,
   sizeof(ns_fir_shibata_11025) / sizeof(double),
   11025,
   _T("[11K]Shibata")
 },
// 8000 Hz -- Shibata; ns_fir() to calculate. [SND_NSHAPE_SHIB8]
 {
   &ns_fir,
   ns_fir_shibata_8000,
   sizeof(ns_fir_shibata_8000) / sizeof(double),
   8000,
   _T("[8K]Shibata")
 },
// 48000 Hz -- Low Shibata; ns_fir() to calculate. [SND_NSHAPE_LOSHIB48]
 {
   &ns_fir,
   ns_fir_low_shibata_48000,
   sizeof(ns_fir_low_shibata_48000) / sizeof(double),
   48000,
   _T("[48K]Low Shibata")
 },
// 44100 Hz -- Low Shibata; ns_fir() to calculate. [SND_NSHAPE_LOSHIB44]
 {
   &ns_fir,
   ns_fir_low_shibata_44100,
   sizeof(ns_fir_low_shibata_44100) / sizeof(double),
   44100,
   _T("[44K]Low Shibata")
 },
// 44100 Hz -- High Shibata; ns_fir() to calculate. [SND_NSHAPE_HISHIB44]
 {
   &ns_fir,
   ns_fir_high_shibata_44100,
   sizeof(ns_fir_high_shibata_44100) / sizeof(double),
   44100,
   _T("[44K]High Shibata")
 },
// 44100 Hz -- Gesemann; ns_iir() to calculate. [SND_NSHAPE_GES44]
 {
   &ns_iir,
   ns_iir_gesemann_44100,
   sizeof(ns_iir_gesemann_44100) / sizeof(double) / 2 /* IIR */,
   44100,
   _T("[44K]Gesemann(IIR)")
 },
// 48000 Hz -- Gesemann; ns_iir() to calculate. [SND_NSHAPE_GES48]
 {
   &ns_iir,
   ns_iir_gesemann_48000,
   sizeof(ns_iir_gesemann_48000) / sizeof(double) / 2 /* IIR */,
   48000,
   _T("[48K]Gesemann(IIR)")
 }
};

/* The noise shaping infrastructure
 * --- ----- ------- --------------
 */
// here we define own sound render filter infrastructure, because the corresponding filters
// came to us "as is" and may have unique procedures for calculation.

/* the "empty" noise shaping filter
*/
static double ns_empty(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes)
{
 return 0.0;                                            // error always zero
}

/* the FIR noise shaping filter
*/
static double ns_fir(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes)
{
#define FES         (fes)
 double res = 0.0;
 unsigned ix_buf;
 unsigned ix_c;

 if(nss -> ns_ix_pos)
  --(nss -> ns_ix_pos);
 else
  nss -> ns_ix_pos = nss -> dsc -> num_coeffs - 1;

 if(!fes -> is_enable)
 {                                                      // == WITHOUT FP CHECKS ==
  nss -> ns_ebuffer[ix_buf = nss -> ns_ix_pos] = val;

  for(ix_c = 0; ix_c < nss -> dsc -> num_coeffs; ++ix_c)
  {
   res += nss -> dsc -> filter_coeffs[ix_c] * nss -> ns_ebuffer[ix_buf];
   if(++ix_buf >= nss -> dsc -> num_coeffs)             // %= VERY slowly
    ix_buf = 0;
  }
 }
 else
 {                                                      // == WITH FC CHECKS ==
  nss -> ns_ebuffer[ix_buf = nss -> ns_ix_pos] = FC(val);

  for(ix_c = 0; ix_c < nss -> dsc -> num_coeffs; ++ix_c)
  {
   res = FC(res + FC(nss -> dsc -> filter_coeffs[ix_c] * nss -> ns_ebuffer[ix_buf]));
   if(++ix_buf >= nss -> dsc -> num_coeffs)             // %= VERY slowly
    ix_buf = 0;
  }
 }

 return res;
#undef FES
}

/* the IIR noise shaping filter
*/
static double ns_iir(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes)
{
#define FES         (fes)
 double res = 0.0;
 unsigned ix_buf;
 unsigned ix_c;

// nss -> dsc -> num_coeffs is filter order. The total ## of coeffs. is (nss->dsc->num_coeffs * 2)

 if(nss -> ns_ix_pos)
  --(nss -> ns_ix_pos);
 else
  nss -> ns_ix_pos = nss -> dsc -> num_coeffs - 1;

 if(!fes -> is_enable)
 {                                                      // == WITHOUT FP CHECKS ==
  nss -> ns_ebuffer[ix_buf = nss -> ns_ix_pos] = val;

  for(ix_c = 0; ix_c < nss -> dsc -> num_coeffs; ++ix_c)
  {
   res += nss -> dsc -> filter_coeffs[ix_c                           ] * nss -> ns_ebuffer[ix_buf]
        - nss -> dsc -> filter_coeffs[ix_c + nss -> dsc -> num_coeffs] * nss -> ns_obuffer[ix_buf];
   if(++ix_buf >= nss -> dsc -> num_coeffs)             // %= VERY slowly
    ix_buf = 0;
  }

  nss -> ns_obuffer[(nss -> ns_ix_pos? nss -> ns_ix_pos : nss -> dsc -> num_coeffs) - 1] = res;
 }
 else
 {                                                      // == WITH FP CHECKS ==
  nss -> ns_ebuffer[ix_buf = nss -> ns_ix_pos] = FC(val);

  for(ix_c = 0; ix_c < nss -> dsc -> num_coeffs; ++ix_c)
  {
   res = FC(res + FC(FC(nss -> dsc -> filter_coeffs[ix_c                           ] * nss -> ns_ebuffer[ix_buf])
        -            FC(nss -> dsc -> filter_coeffs[ix_c + nss -> dsc -> num_coeffs] * nss -> ns_obuffer[ix_buf])));
   if(++ix_buf >= nss -> dsc -> num_coeffs)             // %= VERY slowly
    ix_buf = 0;
  }

  nss -> ns_obuffer[(nss -> ns_ix_pos? nss -> ns_ix_pos : nss -> dsc -> num_coeffs) - 1] = res;
 }

 return res;
#undef FES
}

/* and here
*/

/* Some helper(s)
 * ---- ---------
 */
/* recalculate sound render internals
*/
static void sound_render_recalc(SOUND_RENDER *sr)
{
 // dth_mul = 2**dth_bits - 1, i.e.:
 // dth_bits == 1 -> dth_mul = 1 -> dith. ampl. = +- 1 _TPDF reduced_ LSbits
 // dth_bits == 2 -> dth_mul = 3 -> dith. ampl. = +- 2 _TPDF reduced_ LSbits
 // dth_bits == 3 -> dth_mul = 7 -> dith. ampl. = +- 3 _TPDF reduced_ LSbits
 // .............................................................
 // dth_bits = 15 -> dth_mul = 32767 -> dith. ampl. = +- 15 _TPDF reduced_ LSbits
 // NOTE::We don't take a point about is24bits here -- it's a user response
 sr -> dth_mul = pow(2.0, sr -> config.dth_bits) - 1.0;

 sr -> prev_rnd = 0.0;

 switch(sr -> config.quantz_type)
 {
  case SND_QUANTZ_MID_TREAD:
   sr -> round_offset = 0.5;
   sr -> sign_delta = 0;
   break;

  case SND_QUANTZ_MID_RISER:
  default:
   sr -> round_offset = 0.0;
   sr -> sign_delta = -1;
   break;
 }

 if(sr -> is24bits)
 {
  int64_t hib  = 0x800000LL;                            // 2 ** 23; int64_t to avoid silly analyze msg

  sr -> norm_shift = 24 - sr -> config.sign_bits24;
  hib >>= sr -> norm_shift;

  sr -> hi_bound =  (double) hib;
  sr -> lo_bound = -(double)(hib + 1 + sr -> sign_delta);
  sr -> norm_mul = (sr -> norm_shift < 8)?
    (double)(0x100 >> sr -> norm_shift)
    :
    1.0 / (double)(1ULL << (sr -> norm_shift - 8));     // "1ULL" to avoid silly MS analyze warning
 }
 else
 {
  int64_t hib  = 0x8000LL;                              // 2 ** 15; int64_t to avoid silly analyze msg

  sr -> norm_shift = 16 - sr -> config.sign_bits16;
  hib >>= sr -> norm_shift;

  sr -> hi_bound =  (double) hib;
  sr -> lo_bound = -(double)(hib + 1 + sr -> sign_delta);
  sr -> norm_mul = 1.0 / (double)(1ULL << sr -> norm_shift); // "1ULL" to avoid MS analyze warning
 }
 sr -> lo_bound -= (double)(sr -> sign_delta);

 // noise shaping part
 if(sr -> config.nshape_type > SND_NSHAPE_MAX)          // just in case
  sr -> config.nshape_type = SND_NSHAPE_FLAT;

 if(sr -> ns_shaper.ns_ebuffer)
 {
  free(sr -> ns_shaper.ns_ebuffer);
  sr -> ns_shaper.ns_ebuffer = NULL;
 }
 if(sr -> ns_shaper.ns_obuffer)
 {
  free(sr -> ns_shaper.ns_obuffer);
  sr -> ns_shaper.ns_obuffer = NULL;
 }

 sr -> ns_shaper.dsc = &shaper_dscs[sr -> config.nshape_type];
 if(sr -> ns_shaper.dsc -> num_coeffs)
 {
  unsigned i;

  sr -> ns_shaper.ns_ebuffer = cmalloc(sr -> ns_shaper.dsc -> num_coeffs * sizeof(double));
  sr -> ns_shaper.ns_obuffer = cmalloc(sr -> ns_shaper.dsc -> num_coeffs * sizeof(double));
  for(i = 0; i < sr -> ns_shaper.dsc -> num_coeffs; ++i)
   sr -> ns_shaper.ns_obuffer[i] = sr -> ns_shaper.ns_ebuffer[i] = 0.0;
 }
 sr -> ns_shaper.ns_ix_pos = 0;

 sr -> prev_ns_err = 0.0;
}

/* The Interface
 * --- ---------
 */
/* one-time init the render with the random's seed
*/
void sound_render_init(const SR_VCONFIG *cfg,  int need24bits, uint32_t seed, SOUND_RENDER *sr)
{
 mtrnd_init_seed(&sr -> jrnd, seed);

 sr -> is24bits = need24bits;
 sr -> ns_shaper.ns_ebuffer = NULL;
 sr -> ns_shaper.ns_obuffer = NULL;

 sound_render_setup(cfg, sr);
}

/* sound render deinitialization
*/
void sound_render_cleanup(SOUND_RENDER *sr)
{
 if(sr -> ns_shaper.ns_ebuffer)
 {
  free(sr -> ns_shaper.ns_ebuffer);
  sr -> ns_shaper.ns_ebuffer = NULL;
 }
 if(sr -> ns_shaper.ns_obuffer)
 {
  free(sr -> ns_shaper.ns_obuffer);
  sr -> ns_shaper.ns_obuffer = NULL;
 }
}

/* set sound render to 16/24 output bits -- to be call in safe places (beetween files) only
*/
void sound_render_set_outbits(int need24bits, SOUND_RENDER *sr)
{
 sr -> is24bits = need24bits;
 sound_render_recalc(sr);
}

/* setup the render with new config -- exclude 16/24 bits
*/
void sound_render_setup(const SR_VCONFIG *cfg, SOUND_RENDER *sr)
{
 sound_render_copy_cfg(cfg, &sr -> config);
 sound_render_recalc(sr);
}

/* copy one sound render configuration to another
*/
void sound_render_copy_cfg(const SR_VCONFIG *src, volatile SR_VCONFIG *dst)
{
 *dst = *src;                                   // for the moment the world is simple
}

/* return string list ordered according .quantz_type indexes
*/
const TCHAR **sound_render_get_quantznames(void)
{
 static const TCHAR *quantz_type_names[] =
 {
  _T("Mid Tread"),                              // [SND_QUANTZ_MID_TREAD]
  _T("Mid Riser"),                              // [SND_QUANTZ_MID_RISER]
  NULL
 };
 return quantz_type_names;
}

/* return string list ordered according .render_type indexes
*/
const TCHAR **sound_render_get_rtypenames(void)
{
 static const TCHAR *render_type_names[] =
 {
  _T("Round to integer"),                       // [SND_RENDER_ROUND]
  _T("Rectangular PDF dithering"),              // [SND_RENDER_RPDF]
  _T("Triangular PDF dithering"),               // [SND_RENDER_TPDF]
  _T("Sloped TPDF dithering"),                  // [SND_RENDER_STPDF]
  _T("Gaussian PDF dithering"),                 // [SND_RENDER_GAUSS]
  NULL
 };
 return render_type_names;
}

/* return string list ordered according .nshape_type indexes
*/
const TCHAR **sound_render_get_nshapenames(void)
{
 static const TCHAR *nshape_type_names[NM_SND_NSHAPE + 1];
 int i;

 for(i = 0; i < NM_SND_NSHAPE; ++i)
  nshape_type_names[i] = shaper_dscs[i].shaper_str;

 nshape_type_names[NM_SND_NSHAPE] = NULL;

 return nshape_type_names;
}

/* get the length of output sample (each sound render is one channel only!)
*/
unsigned sound_render_size(SOUND_RENDER *sr)
{
 return sr -> is24bits? 3 : 2;
}

/* render a sample value to buffer (**buf not unsigned due to WinAmp conventions)
*/
void sound_render_value
    ( char **buf
    , double input
    , unsigned *clip_cnt
    , double *peak_val
    , SOUND_RENDER *sr
    , FP_EXCEPT_STATS *fes
    )
{
#define FES         (fes)
 // D[TPDF(-1..1) = 1/6 == this mean SIGMA = 1/SQRT(6) / +- LSBit. We take it as
 // some "etalon" to other dithering distributions:: ==1/SQRT(6) / +- LSBit==

 double rnd_dth = 0.0, tr;
 double qinput;
 int delta, val;

 if(!fes -> is_enable)
 {                                                      // == WITHOUT FP CHECKS ==
  // make a random value to dither
  switch(sr -> config.render_type)
  {
   case SND_RENDER_ROUND:                               // ==0
   default:
    break;

   case SND_RENDER_RPDF:                                // (-1/sqrt(2)..1/sqrt(2))
    rnd_dth = mtrnd_gen_dsopen(&sr -> jrnd) / SQRT2;
    break;

   case SND_RENDER_TPDF:                                // triag (-1..1)
    // D[TPDF(-1..1) = 1/6 == this maen SIGMA = 1/SQRT(6) / +- LSBit
    rnd_dth  = mtrnd_gen_dsopen(&sr -> jrnd);
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);
    rnd_dth /= 2.0;
    break;

   case SND_RENDER_STPDF:                               // sloped triag (-1..1)
    rnd_dth = ((tr = mtrnd_gen_dsopen(&sr -> jrnd)) - sr -> prev_rnd) / 2.0;
    sr -> prev_rnd = tr;
    break;

   case SND_RENDER_GAUSS:
    // we *DON'T NEED* here a "precision" Gaussian Distribution -- it can give unwanted
    // big deviation in single sample; wa take a sum of 12 uniform randoms and
    // normalize it to dispersion == 1. And take int as value for "1 bit" dithering
    rnd_dth  = mtrnd_gen_dsopen(&sr -> jrnd);           // 1
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 2
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 3
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 4
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 5
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 6
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 7
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 8
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 9
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 10
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 11
    rnd_dth += mtrnd_gen_dsopen(&sr -> jrnd);           // 12
    rnd_dth /= (2.0 /*Dispersion[rnd_dth] = 1.0*/ * SQRT6 /*to TPDF eq.*/);
    break;
  }

  // normalize according bits depth and make noise shaping
  input = (input * sr -> norm_mul) - sr -> prev_ns_err;
  qinput = input + (rnd_dth * sr -> dth_mul);

  // convert to integer
  if(qinput < 0.0)
  {
   qinput -= sr -> round_offset;
   delta = sr -> sign_delta;
  }
  else
  {
   qinput += sr -> round_offset;
   delta = 0;
  }

  // update peak level (dB, 0.0 == SR_ZERO_SIGNAL_DB (-555.0) dB)
  if(peak_val)
  {
   double pv = adbl_read(peak_val);
   double cv = fabs(qinput) / sr -> hi_bound;
   
   cv = cv? 20.0 * log10(cv) : SR_ZERO_SIGNAL_DB;

   if(cv > pv)
    adbl_write(peak_val, cv);                                       //  *peak_val = cv;
  }

  // check for clips / saturation
  if(qinput >= sr -> hi_bound)
  {
   qinput =  sr -> hi_bound - 1.0;
   // ++(*clip_cnt)
   if(clip_cnt)
    InterlockedIncrement((volatile LONG *)clip_cnt);
  }
  if(qinput <= sr -> lo_bound)
  {
   qinput = sr -> lo_bound + 1.0;
   // ++(*clip_cnt);
   if(clip_cnt)
    InterlockedIncrement((volatile LONG *)clip_cnt);
  }

  val = ((int)qinput) + delta;

  // noise shaping for the next time
  sr -> prev_ns_err = (*(sr -> ns_shaper.dsc -> ns_filter))((double)val - input, &sr -> ns_shaper, fes);

  // LSB always zero
  val <<= sr -> norm_shift;

  // place into the buffer
  *(*buf)++ = (char)(val);
  *(*buf)++ = (char)(val >> 8);
  if(sr -> is24bits)
   *(*buf)++ = (char)(val >> 16);
 }
 else
 {                                                      // == WITH FP CHECKS ==
  // make a random value to dither
  switch(sr -> config.render_type)
  {
   case SND_RENDER_ROUND:                               // ==0
   default:
    break;

   case SND_RENDER_RPDF:                                // (-1/sqrt(2)..1/sqrt(2))
    rnd_dth = FC(mtrnd_gen_dsopen(&sr -> jrnd) / SQRT2);
    break;

   case SND_RENDER_TPDF:                                // triag (-1..1)
    // D[TPDF(-1..1) = 1/6 == this maen SIGMA = 1/SQRT(6) / +- LSBit
    rnd_dth  = mtrnd_gen_dsopen(&sr -> jrnd);
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));
    rnd_dth = FC(rnd_dth / 2.0);
    break;

   case SND_RENDER_STPDF:                               // sloped triag (-1..1)
    rnd_dth = FC(FC(FC(tr = mtrnd_gen_dsopen(&sr -> jrnd)) - sr -> prev_rnd) / 2.0);
    sr -> prev_rnd = tr;
    break;

   case SND_RENDER_GAUSS:
    // we *DON'T NEED* here a "precision" Gaussian Distribution -- it can give unwanted
    // big deviation in a single sample; wa take a sum of 12 uniform randoms and
    // normalize it to dispersion == 1. And take int as value for "1 bit" dithering
    rnd_dth  = mtrnd_gen_dsopen(&sr -> jrnd);                   // 1
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 2
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 3
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 4
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 5
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 6
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 7
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 8
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 9
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 10
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 11
    rnd_dth = FC(rnd_dth + mtrnd_gen_dsopen(&sr -> jrnd));      // 12
    rnd_dth = FC(rnd_dth / (2.0 /*Dispersion[rnd_dth] = 1.0*/ * SQRT6 /*to TPDF eq.*/));
    break;
  }

  // normalize according bits depth and make noise shaping
  input = FC(FC(input * sr -> norm_mul) - sr -> prev_ns_err);
  qinput = FC(input + FC(rnd_dth * sr -> dth_mul));

  // convert to integer
  if(qinput < 0.0)
  {
   qinput = FC(qinput - sr -> round_offset);
   delta = sr -> sign_delta;
  }
  else
  {
   qinput = FC(qinput + sr -> round_offset);
   delta = 0;
  }

  // update peak level (dB, 0.0 == -150 dB)
  if(peak_val)
  {
   double pv = adbl_read(peak_val);
   double cv = fabs(qinput) / sr -> hi_bound;
   
   cv = cv? 20.0 * log10(cv) : SR_ZERO_SIGNAL_DB;

   if(cv > pv)
    adbl_write(peak_val, cv);                           //  *peak_val = cv;
  }

  // check for clips / saturation
  if(qinput >= sr -> hi_bound)
  {
   qinput =  sr -> hi_bound - 1.0;                      // FC() don't need -- it's precictable constant
   // ++(*clip_cnt)
   if(clip_cnt)
    InterlockedIncrement((volatile LONG *)clip_cnt);
  }
  if(qinput <= sr -> lo_bound)
  {
   qinput = sr -> lo_bound + 1.0;                       // FC() don't need -- it's precictable constant
   // ++(*clip_cnt);
   if(clip_cnt)
    InterlockedIncrement((volatile LONG *)clip_cnt);
  }

  val = ((int)qinput) + delta;

  // noise shaping for the next time
  sr -> prev_ns_err = (*(sr -> ns_shaper.dsc -> ns_filter))(FC((double)val - input), &sr -> ns_shaper, fes);

  // LSB always zero
  val <<= sr -> norm_shift;

  // place into the buffer
  *(*buf)++ = (char)(val);
  *(*buf)++ = (char)(val >> 8);
  if(sr -> is24bits)
   *(*buf)++ = (char)(val >> 16);
 }
#undef FES
}


/* the end...
*/

