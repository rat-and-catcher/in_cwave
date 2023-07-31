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

#include <stdlib.h>

#include "cmalloc.h"        // BOOL type here
#include "hblpf.h"

/* THE PRECALCULATED elliptic Half Band Low Pass Filterss
 * --- ------------- -------- ---- ---- --- ---- --------
 * in_cwave V02.03.01 statement
 * -------- --------- ---------
 * The precalulated filters, presented below, created by program gen-iir,
 * (github.com/rat-and-catcher/gen-iir), which uses the DSPL-2 library by
 * Serge Bakhurin, www.dsplib.org).
 *
 * Old statement
 * --- ---------
 * this filters was calculated by climed prototypes (designs) using third party tool - DSP.DLL,
 * obtained from the site dsplib.ru, where it's lived some time (w/o sources, only API +
 * win32 binary) and was available fo free. DSP.DLL was written by Serge Bakhurin, dsplib.ru.
 * Unfortunally, now this binary unavailable - it was removed from the site by author as "unsupported".
 * But the site still alive, and even has an English version at dsplib.org. We take many great
 * ideas from Serge Bahurin's site materials, especially from his papers -- many thanks for him
 * from Rat and Catcher Technologies.
 *
 * NOTE: we can't get stable elliptic filters of order greater 19 (now-20!) for our needs (whith
 * extremly small transition width + high rejection (>100dB) and acceptable ripple
 * in pass band. May be latter...
 */

// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 15 (16 coefficients in numerator / denominator);
// -- Suppression in stop band 100.0000 dB;
// -- Max. ripple in pass band 1.5000 dB;
// -- Cutoff frequency: 0.495 ([0..1] norm.)
// -- Transition width 0.0099170047 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[16]):
static uint64_t IIR_LOEL_TYPE0_B[] =              // 16 coefficients
  {
    0x3F619AFC9A02BA05U,  // [ 0] dec:  2.14909874201585580E-003; hex:   0X1.19AFC9A02BA050P-9
    0x3F82AF1B1480D3D4U,  // [ 1] dec:  9.12305027314089940E-003; hex:   0X1.2AF1B1480D3D40P-7
    0x3F9E619E38C69849U,  // [ 2] dec:  2.96692583230185080E-002; hex:   0X1.E619E38C698490P-6
    0x3FB16FC2A89F7068U,  // [ 3] dec:  6.81115781475810640E-002; hex:   0X1.16FC2A89F70680P-4
    0x3FC0957CE6EB0C56U,  // [ 4] dec:  1.29562008622175540E-001; hex:   0X1.0957CE6EB0C560P-3
    0x3FC9FFC923AB0CA9U,  // [ 5] dec:  2.03118460098356910E-001; hex:   0X1.9FFC923AB0CA90P-3
    0x3FD17881D051D3ADU,  // [ 6] dec:  2.72980168759484000E-001; hex:   0X1.17881D051D3AD0P-2
    0x3FD42A1437ED7200U,  // [ 7] dec:  3.15068297020644650E-001; hex:   0X1.42A1437ED72000P-2
    0x3FD42A1437ED7200U,  // [ 8] dec:  3.15068297020644650E-001; hex:   0X1.42A1437ED72000P-2
    0x3FD17881D051D3ADU,  // [ 9] dec:  2.72980168759484000E-001; hex:   0X1.17881D051D3AD0P-2
    0x3FC9FFC923AB0CA9U,  // [10] dec:  2.03118460098356910E-001; hex:   0X1.9FFC923AB0CA90P-3
    0x3FC0957CE6EB0C56U,  // [11] dec:  1.29562008622175540E-001; hex:   0X1.0957CE6EB0C560P-3
    0x3FB16FC2A89F7068U,  // [12] dec:  6.81115781475810640E-002; hex:   0X1.16FC2A89F70680P-4
    0x3F9E619E38C69849U,  // [13] dec:  2.96692583230185080E-002; hex:   0X1.E619E38C698490P-6
    0x3F82AF1B1480D3D4U,  // [14] dec:  9.12305027314089940E-003; hex:   0X1.2AF1B1480D3D40P-7
    0x3F619AFC9A02BA05U   // [15] dec:  2.14909874201585580E-003; hex:   0X1.19AFC9A02BA050P-9
  };
// Denominator coefficients (A[16]):
static uint64_t IIR_LOEL_TYPE0_A[] =              // 16 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC00BBE1E2A7C11A7U,  // [ 1] dec: -3.46783097449425130E+000; hex:  -0X1.BBE1E2A7C11A70P+1
    0x4025A35A322FB995U,  // [ 2] dec:  1.08190475161798450E+001; hex:   0X1.5A35A322FB9950P+3
    0xC0367621811D2F5AU,  // [ 3] dec: -2.24614487358333930E+001; hex:  -0X1.67621811D2F5A0P+4
    0x4044411BAB3D802BU,  // [ 4] dec:  4.05086568880363630E+001; hex:   0X1.4411BAB3D802B0P+5
    0xC04D84F808E4DB7FU,  // [ 5] dec: -5.90388194196193510E+001; hex:  -0X1.D84F808E4DB7F0P+5
    0x4052C36C169DAE7BU,  // [ 6] dec:  7.50534721889361410E+001; hex:   0X1.2C36C169DAE7B0P+6
    0xC05452666489F59DU,  // [ 7] dec: -8.12874995562801390E+001; hex:  -0X1.452666489F59D0P+6
    0x405333CC1E26D46DU,  // [ 8] dec:  7.68093333605954310E+001; hex:   0X1.333CC1E26D46D0P+6
    0xC04F45FACA091E73U,  // [ 9] dec: -6.25467159790540780E+001; hex:  -0X1.F45FACA091E730P+5
    0x4045F225DA5BF82DU,  // [10] dec:  4.38917801808423530E+001; hex:   0X1.5F225DA5BF82D0P+5
    0xC03A2E9885BDCC38U,  // [11] dec: -2.61820148075459830E+001; hex:  -0X1.A2E9885BDCC380P+4
    0x4029E121FF4841FDU,  // [12] dec:  1.29397125030236510E+001; hex:   0X1.9E121FF4841FD0P+3
    0xC014B2BF6CC051ACU,  // [13] dec: -5.17455835269758420E+000; hex:  -0X1.4B2BF6CC051AC0P+2
    0x3FF7E50A33B0A073U,  // [14] dec:  1.49341793242822680E+000; hex:   0X1.7E50A33B0A0730P+0
    0xBFD30189DD3C9C54U   // [15] dec: -2.96968904544376190E-001; hex:  -0X1.30189DD3C9C540P-2
  };

// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 19 (20 coefficients in numerator / denominator);
// -- Suppression in stop band 100.0000 dB;
// -- Max. ripple in pass band 1.8000 dB;
// -- Cutoff frequency: 0.499 ([0..1] norm.)
// -- Transition width 0.002131305 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[20]):
static uint64_t IIR_LOEL_TYPE1_B[] =              // 20 coefficients
  {
    0x3F61616AB173668BU,  // [ 0] dec:  2.12164724835590050E-003; hex:   0X1.1616AB173668B0P-9
    0x3F825102375E9B13U,  // [ 1] dec:  8.94357425188511100E-003; hex:   0X1.25102375E9B130P-7
    0x3FA10E931F1BEAB5U,  // [ 2] dec:  3.33143210863985700E-002; hex:   0X1.10E931F1BEAB50P-5
    0x3FB5A04D2843C950U,  // [ 3] dec:  8.44772551825248020E-002; hex:   0X1.5A04D2843C9500P-4
    0x3FC7EB1F823D4892U,  // [ 4] dec:  1.86862886994792420E-001; hex:   0X1.7EB1F823D48920P-3
    0x3FD5C96C08823278U,  // [ 5] dec:  3.40418823537028010E-001; hex:   0X1.5C96C088232780P-2
    0x3FE18EDE760081FDU,  // [ 6] dec:  5.48690062019943190E-001; hex:   0X1.18EDE760081FD0P-1
    0x3FE8A8C0817B920CU,  // [ 7] dec:  7.70599606414692940E-001; hex:   0X1.8A8C0817B920C0P-1
    0x3FEEF08441994DDDU,  // [ 8] dec:  9.66859939692252040E-001; hex:   0X1.EF08441994DDD0P-1
    0x3FF145DAFF26CB16U,  // [ 9] dec:  1.07955455463951950E+000; hex:   0X1.145DAFF26CB160P+0
    0x3FF145DAFF26CB16U,  // [10] dec:  1.07955455463951950E+000; hex:   0X1.145DAFF26CB160P+0
    0x3FEEF08441994DDDU,  // [11] dec:  9.66859939692252040E-001; hex:   0X1.EF08441994DDD0P-1
    0x3FE8A8C0817B920CU,  // [12] dec:  7.70599606414692940E-001; hex:   0X1.8A8C0817B920C0P-1
    0x3FE18EDE760081FDU,  // [13] dec:  5.48690062019943190E-001; hex:   0X1.18EDE760081FD0P-1
    0x3FD5C96C08823278U,  // [14] dec:  3.40418823537028010E-001; hex:   0X1.5C96C088232780P-2
    0x3FC7EB1F823D4892U,  // [15] dec:  1.86862886994792420E-001; hex:   0X1.7EB1F823D48920P-3
    0x3FB5A04D2843C950U,  // [16] dec:  8.44772551825248020E-002; hex:   0X1.5A04D2843C9500P-4
    0x3FA10E931F1BEAB5U,  // [17] dec:  3.33143210863985700E-002; hex:   0X1.10E931F1BEAB50P-5
    0x3F825102375E9B13U,  // [18] dec:  8.94357425188511100E-003; hex:   0X1.25102375E9B130P-7
    0x3F61616AB173668BU   // [19] dec:  2.12164724835590050E-003; hex:   0X1.1616AB173668B0P-9
  };
// Denominator coefficients (A[20]):
static uint64_t IIR_LOEL_TYPE1_A[] =              // 20 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC00BE310CADB1EDCU,  // [ 1] dec: -3.48587187272265280E+000; hex:  -0X1.BE310CADB1EDC0P+1
    0x4029CB562280899EU,  // [ 2] dec:  1.28971415311723910E+001; hex:   0X1.9CB562280899E0P+3
    0xC03DAD9F07D88A8FU,  // [ 3] dec: -2.96782078651108880E+001; hex:  -0X1.DAD9F07D88A8F0P+4
    0x404FED34774B7273U,  // [ 4] dec:  6.38531636351007350E+001; hex:   0X1.FED34774B72730P+5
    0xC05B3C73E21E9301U,  // [ 5] dec: -1.08944572954042100E+002; hex:  -0X1.B3C73E21E93010P+6
    0x406532CAC3A7048FU,  // [ 6] dec:  1.69587251497465760E+002; hex:   0X1.532CAC3A7048F0P+7
    0xC06C4112B1B044ECU,  // [ 7] dec: -2.26033531994129020E+002; hex:  -0X1.C4112B1B044EC0P+7
    0x40711247B1FD71EDU,  // [ 8] dec:  2.73142503728885290E+002; hex:   0X1.11247B1FD71ED0P+8
    0xC07231202413D0B8U,  // [ 9] dec: -2.91070346906093160E+002; hex:  -0X1.231202413D0B80P+8
    0x40717F31DAC223FEU,  // [10] dec:  2.79949671514856050E+002; hex:   0X1.17F31DAC223FE0P+8
    0xC06DF42A52AB6021U,  // [11] dec: -2.39630166372983720E+002; hex:  -0X1.DF42A52AB60210P+7
    0x4066EFE838B0EFB9U,  // [12] dec:  1.83497097344950620E+002; hex:   0X1.6EFE838B0EFB90P+7
    0xC05F261632432921U,  // [13] dec: -1.24595104756914690E+002; hex:  -0X1.F2616324329210P+6
    0x40528D2F7C13B07EU,  // [14] dec:  7.42060232345847620E+001; hex:   0X1.28D2F7C13B07E0P+6
    0xC043456DD83FFF9DU,  // [15] dec: -3.85424146950237870E+001; hex:  -0X1.3456DD83FFF9D0P+5
    0x4030BC445544A337U,  // [16] dec:  1.67354176800319830E+001; hex:   0X1.0BC445544A3370P+4
    0xC0187118078E2617U,  // [17] dec: -6.11044322781665180E+000; hex:  -0X1.87118078E26170P+2
    0x3FF968D5E4E055D0U,  // [18] dec:  1.58809461026008950E+000; hex:   0X1.968D5E4E055D00P+0
    0xBFD49BF4B34C446EU   // [19] dec: -3.22018790336250470E-001; hex:  -0X1.49BF4B34C446E0P-2
  };

// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 18 (19 coefficients in numerator / denominator);
// -- Suppression in stop band 90.0000 dB;
// -- Max. ripple in pass band 2.0000 dB;
// -- Cutoff frequency: 0.499 ([0..1] norm.)
// -- Transition width 0.0015685821 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[19]):
static uint64_t IIR_LOEL_TYPE2_B[] =              // 19 coefficients
  {
    0x3F6E9A7EF6B7FF9BU,  // [ 0] dec:  3.73577878576720820E-003; hex:   0X1.E9A7EF6B7FF9B0P-9
    0x3F8D2E91CD23A859U,  // [ 1] dec:  1.42489805916349730E-002; hex:   0X1.D2E91CD23A8590P-7
    0x3FAA79F8E2F72579U,  // [ 2] dec:  5.17118241364852920E-002; hex:   0X1.A79F8E2F725790P-5
    0x3FBF96D75511111CU,  // [ 3] dec:  1.23395403164128610E-001; hex:   0X1.F96D75511111C0P-4
    0x3FD0D878B78448EDU,  // [ 4] dec:  2.63212374892772880E-001; hex:   0X1.0D878B78448ED0P-2
    0x3FDD287406E7F1EBU,  // [ 5] dec:  4.55594069236922190E-001; hex:   0X1.D287406E7F1EB0P-2
    0x3FE688A02DE3322CU,  // [ 6] dec:  7.04177941917412560E-001; hex:   0X1.688A02DE3322C0P-1
    0x3FEE12DA8CB080AEU,  // [ 7] dec:  9.39801478180035010E-001; hex:   0X1.E12DA8CB080AE0P-1
    0x3FF1FF5095FF5225U,  // [ 8] dec:  1.12483271210397470E+000; hex:   0X1.1FF5095FF52250P+0
    0x3FF2FCC12CAA3DBCU,  // [ 9] dec:  1.18670766303317430E+000; hex:   0X1.2FCC12CAA3DBC0P+0
    0x3FF1FF5095FF5225U,  // [10] dec:  1.12483271210397470E+000; hex:   0X1.1FF5095FF52250P+0
    0x3FEE12DA8CB080AEU,  // [11] dec:  9.39801478180035010E-001; hex:   0X1.E12DA8CB080AE0P-1
    0x3FE688A02DE3322CU,  // [12] dec:  7.04177941917412560E-001; hex:   0X1.688A02DE3322C0P-1
    0x3FDD287406E7F1EBU,  // [13] dec:  4.55594069236922190E-001; hex:   0X1.D287406E7F1EB0P-2
    0x3FD0D878B78448EDU,  // [14] dec:  2.63212374892772880E-001; hex:   0X1.0D878B78448ED0P-2
    0x3FBF96D75511111CU,  // [15] dec:  1.23395403164128610E-001; hex:   0X1.F96D75511111C0P-4
    0x3FAA79F8E2F72579U,  // [16] dec:  5.17118241364852920E-002; hex:   0X1.A79F8E2F725790P-5
    0x3F8D2E91CD23A859U,  // [17] dec:  1.42489805916349730E-002; hex:   0X1.D2E91CD23A8590P-7
    0x3F6E9A7EF6B7FF9BU   // [18] dec:  3.73577878576720820E-003; hex:   0X1.E9A7EF6B7FF9B0P-9
  };
// Denominator coefficients (A[19]):
static uint64_t IIR_LOEL_TYPE2_A[] =              // 19 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC0092AC40137416AU,  // [ 1] dec: -3.14588166189075920E+000; hex:  -0X1.92AC40137416A0P+1
    0x4027069435724C96U,  // [ 2] dec:  1.15128494932198850E+001; hex:   0X1.7069435724C960P+3
    0xC038C950A3E8D905U,  // [ 3] dec: -2.47863867228961860E+001; hex:  -0X1.8C950A3E8D9050P+4
    0x4049F5E6E468F2DAU,  // [ 4] dec:  5.19211087715572860E+001; hex:   0X1.9F5E6E468F2DA0P+5
    0xC054F78F96DC2BDCU,  // [ 5] dec: -8.38681389951811410E+001; hex:  -0X1.4F78F96DC2BDC0P+6
    0x405F784A718D036FU,  // [ 6] dec:  1.25879543674190910E+002; hex:   0X1.F784A718D036F0P+6
    0xC063E5A30D375A29U,  // [ 7] dec: -1.59176153762922040E+002; hex:  -0X1.3E5A30D375A290P+7
    0x4066FD0682B38D64U,  // [ 8] dec:  1.83907044745147800E+002; hex:   0X1.6FD0682B38D640P+7
    0xC06725D13A955B7AU,  // [ 9] dec: -1.85181790630067380E+002; hex:  -0X1.725D13A955B7A0P+7
    0x4065149F4F65FBA6U,  // [10] dec:  1.68644447039781140E+002; hex:   0X1.5149F4F65FBA60P+7
    0xC060DF1ECDAC6007U,  // [11] dec: -1.34972510182066090E+002; hex:  -0X1.0DF1ECDAC60070P+7
    0x4058250DE3176D79U,  // [12] dec:  9.65789725998673840E+001; hex:   0X1.8250DE3176D790P+6
    0xC04DFF728CAC97FBU,  // [13] dec: -5.99956832735769790E+001; hex:  -0X1.DFF728CAC97FB0P+5
    0x40405D935F6F80C6U,  // [14] dec:  3.27310599607895230E+001; hex:   0X1.05D935F6F80C60P+5
    0xC02D9533B6F792BFU,  // [15] dec: -1.47914101769650850E+001; hex:  -0X1.D9533B6F792BF0P+3
    0x4016CCDD51142D93U,  // [16] dec:  5.70006300626427540E+000; hex:   0X1.6CCDD51142D930P+2
    0xBFF88F38CE4768E9U,  // [17] dec: -1.53496628358566970E+000; hex:  -0X1.88F38CE4768E90P+0
    0x3FD5B6E90480857FU   // [18] dec:  3.39288954159279340E-001; hex:   0X1.5B6E90480857F0P-2
  };

// !!THE UGLY FILTER -- NOT FOR CONVENTIONAL USE -- ONLY FOR EXPERIMENTS --
// -- THE GUY IS ABOUT GENERATION NEAR CUTOFF FREUENCY!!
// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 19 (20 coefficients in numerator / denominator);
// -- Suppression in stop band 96.0000 dB;
// -- Max. ripple in pass band 2.0000 dB;
// -- Cutoff frequency: 0.4982 ([0..1] norm.)
// -- Transition width 0.0015899102 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[20]):
static uint64_t IIR_LOEL_TYPE3_B[] =		// 20 coefficients
  {
    0x3F6576F5632D5F44U,  // [ 0] dec:  2.62020041683888770E-003; hex:   0X1.576F5632D5F440P-9
    0x3F856D2EC232921DU,  // [ 1] dec:  1.04621556295763590E-002; hex:   0X1.56D2EC232921D0P-7
    0x3FA400328D1571C7U,  // [ 2] dec:  3.90640065404315660E-002; hex:   0X1.400328D1571C70P-5
    0x3FB8D24D69D7027DU,  // [ 3] dec:  9.69589599035404900E-002; hex:   0X1.8D24D69D7027D0P-4
    0x3FCB65B7E3A59ADCU,  // [ 4] dec:  2.14041696696958610E-001; hex:   0X1.B65B7E3A59ADC0P-3
    0x3FD8B2ED62D87921U,  // [ 5] dec:  3.85920855072884150E-001; hex:   0X1.8B2ED62D879210P-2
    0x3FE3DC4495952C66U,  // [ 6] dec:  6.20638172296264610E-001; hex:   0X1.3DC4495952C660P-1
    0x3FEBC12160D83B5AU,  // [ 7] dec:  8.67325486325948750E-001; hex:   0X1.BC12160D83B5A0P-1
    0x3FF1639B9928E633U,  // [ 8] dec:  1.08681831195862060E+000; hex:   0X1.1639B9928E6330P+0
    0x3FF3630A01EAF8A6U,  // [ 9] dec:  1.21167946576273660E+000; hex:   0X1.3630A01EAF8A60P+0
    0x3FF3630A01EAF8A6U,  // [10] dec:  1.21167946576273660E+000; hex:   0X1.3630A01EAF8A60P+0
    0x3FF1639B9928E633U,  // [11] dec:  1.08681831195862060E+000; hex:   0X1.1639B9928E6330P+0
    0x3FEBC12160D83B5AU,  // [12] dec:  8.67325486325948750E-001; hex:   0X1.BC12160D83B5A0P-1
    0x3FE3DC4495952C66U,  // [13] dec:  6.20638172296264610E-001; hex:   0X1.3DC4495952C660P-1
    0x3FD8B2ED62D87921U,  // [14] dec:  3.85920855072884150E-001; hex:   0X1.8B2ED62D879210P-2
    0x3FCB65B7E3A59ADCU,  // [15] dec:  2.14041696696958610E-001; hex:   0X1.B65B7E3A59ADC0P-3
    0x3FB8D24D69D7027DU,  // [16] dec:  9.69589599035404900E-002; hex:   0X1.8D24D69D7027D0P-4
    0x3FA400328D1571C7U,  // [17] dec:  3.90640065404315660E-002; hex:   0X1.400328D1571C70P-5
    0x3F856D2EC232921DU,  // [18] dec:  1.04621556295763590E-002; hex:   0X1.56D2EC232921D0P-7
    0x3F6576F5632D5F44U   // [19] dec:  2.62020041683888770E-003; hex:   0X1.576F5632D5F440P-9
  };
// Denominator coefficients (A[20]):
static uint64_t IIR_LOEL_TYPE3_A[] =		// 20 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC00B41B2DE950929U,  // [ 1] dec: -3.40707944767304530E+000; hex:  -0X1.B41B2DE9509290P+1
    0x402970A0EA172D03U,  // [ 2] dec:  1.27199776795664600E+001; hex:   0X1.970A0EA172D030P+3
    0xC03CFED84AB0ED47U,  // [ 3] dec: -2.89954878503752090E+001; hex:  -0X1.CFED84AB0ED470P+4
    0x404F590317F3C410U,  // [ 4] dec:  6.26954069080603630E+001; hex:   0X1.F590317F3C4100P+5
    0xC05AA35640A0E00EU,  // [ 5] dec: -1.06552139432053280E+002; hex:  -0X1.AA35640A0E00E0P+6
    0x4064CDC8BD4B0A49U,  // [ 6] dec:  1.66430754324496120E+002; hex:   0X1.4CDC8BD4B0A490P+7
    0xC06BB310D9003B3DU,  // [ 7] dec: -2.21595806599094350E+002; hex:  -0X1.BB310D9003B3D0P+7
    0x4070C8F6C4CDF4A4U,  // [ 8] dec:  2.68560246281160520E+002; hex:   0X1.0C8F6C4CDF4A40P+8
    0xC071E67A4835A0F8U,  // [ 9] dec: -2.86404854020583570E+002; hex:  -0X1.1E67A4835A0F80P+8
    0x407144517F3D71B7U,  // [10] dec:  2.76269896736160660E+002; hex:   0X1.144517F3D71B70P+8
    0xC06D9FEC7452436AU,  // [11] dec: -2.36997614060087760E+002; hex:  -0X1.D9FEC7452436A0P+7
    0x4066C2447072BF85U,  // [12] dec:  1.82070854400746750E+002; hex:   0X1.6C2447072BF850P+7
    0xC05F058A09B24BB4U,  // [13] dec: -1.24086550163380540E+002; hex:  -0X1.F058A09B24BB40P+6
    0x40528B6CE7AB88B9U,  // [14] dec:  7.41785220313203270E+001; hex:   0X1.28B6CE7AB88B90P+6
    0xC043617A896818BFU,  // [15] dec: -3.87615520246777050E+001; hex:  -0X1.3617A896818BF0P+5
    0x4030E63ADAFAABE6U,  // [16] dec:  1.68993355619421880E+001; hex:   0X1.0E63ADAFAABE60P+4
    0xC018F5717FC5675BU,  // [17] dec: -6.23969077722889680E+000; hex:  -0X1.8F5717FC5675B0P+2
    0x3FFA06EE9694BC96U,  // [18] dec:  1.62669237919525280E+000; hex:   0X1.A06EE9694BC960P+0
    0xBFD5C02814FC1467U   // [19] dec: -3.39853306286676150E-001; hex:  -0X1.5C02814FC14670P-2
  };

// !!THE UGLY FILTER -- NOT FOR CONVENTIONAL USE -- ONLY FOR EXPERIMENTS --
// -- THE GUY IS ABOUT GENERATION NEAR CUTOFF FREUENCY!!
// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 20 (21 coefficients in numerator / denominator);
// -- Suppression in stop band 100.0000 dB;
// -- Max. ripple in pass band 2.0000 dB;
// -- Cutoff frequency: 0.4982 ([0..1] norm.)
// -- Transition width 0.001414773 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[21]):
static uint64_t IIR_LOEL_TYPE4_B[] =		// 21 coefficients
  {
    0x3F6105CC7C5CBC69U,  // [ 0] dec:  2.07796038275591790E-003; hex:   0X1.105CC7C5CBC690P-9
    0x3F81955643AB4B9DU,  // [ 1] dec:  8.58561891565106680E-003; hex:   0X1.1955643AB4B9D0P-7
    0x3FA0E59FD9ABE75AU,  // [ 2] dec:  3.30018952572134900E-002; hex:   0X1.0E59FD9ABE75A0P-5
    0x3FB5AE5A762E5FB7U,  // [ 3] dec:  8.46916712310975400E-002; hex:   0X1.5AE5A762E5FB70P-4
    0x3FC8BDB6858FC0F8U,  // [ 4] dec:  1.93289580550761060E-001; hex:   0X1.8BDB6858FC0F80P-3
    0x3FD71EFDB4D343E6U,  // [ 5] dec:  3.61266542994654550E-001; hex:   0X1.71EFDB4D343E60P-2
    0x3FE350C40F25D47BU,  // [ 6] dec:  6.03609113297934540E-001; hex:   0X1.350C40F25D47B0P-1
    0x3FEC1A5CD098A437U,  // [ 7] dec:  8.78218085684358550E-001; hex:   0X1.C1A5CD098A4370P-1
    0x3FF26A2239023446U,  // [ 8] dec:  1.15091154355038010E+000; hex:   0X1.26A22390234460P+0
    0x3FF5809599CEFF59U,  // [ 9] dec:  1.34389267045476960E+000; hex:   0X1.5809599CEFF590P+0
    0x3FF6B599D324228FU,  // [10] dec:  1.41933615185749650E+000; hex:   0X1.6B599D324228F0P+0
    0x3FF5809599CEFF59U,  // [11] dec:  1.34389267045476960E+000; hex:   0X1.5809599CEFF590P+0
    0x3FF26A2239023446U,  // [12] dec:  1.15091154355038010E+000; hex:   0X1.26A22390234460P+0
    0x3FEC1A5CD098A437U,  // [13] dec:  8.78218085684358550E-001; hex:   0X1.C1A5CD098A4370P-1
    0x3FE350C40F25D47BU,  // [14] dec:  6.03609113297934540E-001; hex:   0X1.350C40F25D47B0P-1
    0x3FD71EFDB4D343E6U,  // [15] dec:  3.61266542994654550E-001; hex:   0X1.71EFDB4D343E60P-2
    0x3FC8BDB6858FC0F8U,  // [16] dec:  1.93289580550761060E-001; hex:   0X1.8BDB6858FC0F80P-3
    0x3FB5AE5A762E5FB7U,  // [17] dec:  8.46916712310975400E-002; hex:   0X1.5AE5A762E5FB70P-4
    0x3FA0E59FD9ABE75AU,  // [18] dec:  3.30018952572134900E-002; hex:   0X1.0E59FD9ABE75A0P-5
    0x3F81955643AB4B9DU,  // [19] dec:  8.58561891565106680E-003; hex:   0X1.1955643AB4B9D0P-7
    0x3F6105CC7C5CBC69U   // [20] dec:  2.07796038275591790E-003; hex:   0X1.105CC7C5CBC690P-9
  };
// Denominator coefficients (A[21]):
static uint64_t IIR_LOEL_TYPE4_A[] =		// 21 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC00C7D82D1B03B10U,  // [ 1] dec: -3.56128467387259920E+000; hex:  -0X1.C7D82D1B03B100P+1
    0x402B532367BD1433U,  // [ 2] dec:  1.36623794954594810E+001; hex:   0X1.B532367BD14330P+3
    0xC0403152BB93B333U,  // [ 3] dec: -3.23853373023215670E+001; hex:  -0X1.03152BB93B3330P+5
    0x40521900A7EFCE19U,  // [ 4] dec:  7.23906650392385840E+001; hex:   0X1.21900A7EFCE190P+6
    0xC0600131C59BF008U,  // [ 5] dec: -1.28037325672689120E+002; hex:  -0X1.00131C59BF0080P+7
    0x4069FD3A9FD5A9DEU,  // [ 6] dec:  2.07913406293212520E+002; hex:   0X1.9FD3A9FD5A9DE0P+7
    0xC07212F5FE6A199CU,  // [ 7] dec: -2.89185057081654800E+002; hex:  -0X1.212F5FE6A199C0P+8
    0x4076ED28243B6725U,  // [ 8] dec:  3.66822300178568470E+002; hex:   0X1.6ED28243B67250P+8
    0xC079B4038C1A4996U,  // [ 9] dec: -4.11250866034207660E+002; hex:  -0X1.9B4038C1A49960P+8
    0x407A30F362603582U,  // [10] dec:  4.19059419990364520E+002; hex:   0X1.A30F3626035820P+8
    0xC077D86046C38B7FU,  // [11] dec: -3.81523504985663690E+002; hex:  -0X1.7D86046C38B7F0P+8
    0x4073A0AD10AEDD99U,  // [12] dec:  3.14042252238339240E+002; hex:   0X1.3A0AD10AEDD990P+8
    0xC06CCD2771406576U,  // [13] dec: -2.30411064744733890E+002; hex:  -0X1.CCD27714065760P+7
    0x4062E94268741CDCU,  // [14] dec:  1.51289356447966270E+002; hex:   0X1.2E94268741CDC0P+7
    0xC055C1A3C0241523U,  // [15] dec: -8.70256195404404450E+001; hex:  -0X1.5C1A3C02415230P+6
    0x4045FB3DD7A2C3B9U,  // [16] dec:  4.39628247780560240E+001; hex:   0X1.5FB3DD7A2C3B90P+5
    0xC03289B60654179EU,  // [17] dec: -1.85379337268164970E+001; hex:  -0X1.289B60654179E0P+4
    0x401A888D2AA55875U,  // [18] dec:  6.63335100778760010E+000; hex:   0X1.A888D2AA558750P+2
    0xBFFADD00E7E16D4AU,  // [19] dec: -1.67895594194745220E+000; hex:  -0X1.ADD00E7E16D4A0P+0
    0x3FD5C0724C400EABU   // [20] dec:  3.39871000731572340E-001; hex:   0X1.5C0724C400EAB0P-2
  };

// !!THE UGLY FILTER -- NOT FOR CONVENTIONAL USE -- ONLY FOR EXPERIMENTS --
// -- THE GUY IS ABOUT GENERATION NEAR CUTOFF FREUENCY!!
// -- Type: LPF; Approximation: ELLIP;
// -- Filter order: 20 (21 coefficients in numerator / denominator);
// -- Suppression in stop band 100.0000 dB;
// -- Max. ripple in pass band 2.0000 dB;
// -- Cutoff frequency: 0.4985 ([0..1] norm.)
// -- Transition width 0.001414773 (??unsure-R&C) ([0..1] norm.)
// Numerator coefficients (B[21]):
static uint64_t IIR_LOEL_TYPE5_B[] =		// 21 coefficients
  {
    0x3F6115A0362A0507U,  // [ 0] dec:  2.08550731014212370E-003; hex:   0X1.115A0362A05070P-9
    0x3F81B78F1BF99472U,  // [ 1] dec:  8.65089229700741550E-003; hex:   0X1.1B78F1BF994720P-7
    0x3FA106C68AA3F2F5U,  // [ 2] dec:  3.32548183668900720E-002; hex:   0X1.106C68AA3F2F50P-5
    0x3FB5E2194F93B441U,  // [ 3] dec:  8.54812449722013450E-002; hex:   0X1.5E2194F93B4410P-4
    0x3FC8FABE73E9B77BU,  // [ 4] dec:  1.95152098272440930E-001; hex:   0X1.8FABE73E9B77B0P-3
    0x3FD75D0F146BEAC6U,  // [ 5] dec:  3.65054864828988080E-001; hex:   0X1.75D0F146BEAC60P-2
    0x3FE385FBE5F6244BU,  // [ 6] dec:  6.10105466025865240E-001; hex:   0X1.385FBE5F6244B0P-1
    0x3FEC6B051C1551CCU,  // [ 7] dec:  8.88063959932475110E-001; hex:   0X1.C6B051C1551CC0P-1
    0x3FF29FB67CC742FDU,  // [ 8] dec:  1.16399239293622190E+000; hex:   0X1.29FB67CC742FD0P+0
    0x3FF5C037D3726560U,  // [ 9] dec:  1.35942823978833620E+000; hex:   0X1.5C037D37265600P+0
    0x3FF6F8C9E81EBA61U,  // [10] dec:  1.43573942825209880E+000; hex:   0X1.6F8C9E81EBA610P+0
    0x3FF5C037D3726560U,  // [11] dec:  1.35942823978833620E+000; hex:   0X1.5C037D37265600P+0
    0x3FF29FB67CC742FDU,  // [12] dec:  1.16399239293622190E+000; hex:   0X1.29FB67CC742FD0P+0
    0x3FEC6B051C1551CCU,  // [13] dec:  8.88063959932475110E-001; hex:   0X1.C6B051C1551CC0P-1
    0x3FE385FBE5F6244BU,  // [14] dec:  6.10105466025865240E-001; hex:   0X1.385FBE5F6244B0P-1
    0x3FD75D0F146BEAC6U,  // [15] dec:  3.65054864828988080E-001; hex:   0X1.75D0F146BEAC60P-2
    0x3FC8FABE73E9B77BU,  // [16] dec:  1.95152098272440930E-001; hex:   0X1.8FABE73E9B77B0P-3
    0x3FB5E2194F93B441U,  // [17] dec:  8.54812449722013450E-002; hex:   0X1.5E2194F93B4410P-4
    0x3FA106C68AA3F2F5U,  // [18] dec:  3.32548183668900720E-002; hex:   0X1.106C68AA3F2F50P-5
    0x3F81B78F1BF99472U,  // [19] dec:  8.65089229700741550E-003; hex:   0X1.1B78F1BF994720P-7
    0x3F6115A0362A0507U   // [20] dec:  2.08550731014212370E-003; hex:   0X1.115A0362A05070P-9
  };
// Denominator coefficients (A[21]):
static uint64_t IIR_LOEL_TYPE5_A[] =		// 21 coefficients
  {
    0x3FF0000000000000U,  // [ 0] dec:  1.00000000000000000E+000; hex:   0X1.00000000000000P+0
    0xC00C5C130ACD4DE0U,  // [ 1] dec: -3.54495819510496800E+000; hex:  -0X1.C5C130ACD4DE00P+1
    0x402B372B4460A8BDU,  // [ 2] dec:  1.36077519767753700E+001; hex:   0X1.B372B4460A8BD0P+3
    0xC04018067D5C7BDDU,  // [ 3] dec: -3.21876980496960880E+001; hex:  -0X1.018067D5C7BDD0P+5
    0x4051FCF1B08ABAFBU,  // [ 4] dec:  7.19522515635289180E+001; hex:   0X1.1FCF1B08ABAFB0P+6
    0xC05FC7BF1760A08BU,  // [ 5] dec: -1.27121038288462090E+002; hex:  -0X1.FC7BF1760A08B0P+6
    0x4069CD106F19F99FU,  // [ 6] dec:  2.06408256102306920E+002; hex:   0X1.9CD106F19F99F0P+7
    0xC071EEC5687BAA7CU,  // [ 7] dec: -2.86923195345945490E+002; hex:  -0X1.1EEC5687BAA7C0P+8
    0x4076BEEACC80026FU,  // [ 8] dec:  3.63932323932683120E+002; hex:   0X1.6BEEACC80026F0P+8
    0xC0797E7581F3DFBEU,  // [ 9] dec: -4.07903688385613240E+002; hex:  -0X1.97E7581F3DFBE0P+8
    0x4079FAA5C67F698BU,  // [10] dec:  4.15665472505287370E+002; hex:   0X1.9FAA5C67F698B0P+8
    0xC077A6C923C34A3BU,  // [11] dec: -3.78424106371737880E+002; hex:  -0X1.7A6C923C34A3B0P+8
    0x407378C63DFAEC24U,  // [12] dec:  3.11548398952642170E+002; hex:   0X1.378C63DFAEC240P+8
    0xC06C93E512E1F2B0U,  // [13] dec: -2.28621713105492290E+002; hex:  -0X1.C93E512E1F2B00P+7
    0x4062C57E1067F622U,  // [14] dec:  1.50171638682412830E+002; hex:   0X1.2C57E1067F6220P+7
    0xC0559A827DE298E5U,  // [15] dec: -8.64142145836700170E+001; hex:  -0X1.59A827DE298E50P+6
    0x4045D786848EB6D3U,  // [16] dec:  4.36837926575514290E+001; hex:   0X1.5D786848EB6D30P+5
    0xC0326E1D8797A18BU,  // [17] dec: -1.84301380868142070E+001; hex:  -0X1.26E1D8797A18B0P+4
    0x401A69C6F9E351C1U,  // [18] dec:  6.60329809617945870E+000; hex:   0X1.A69C6F9E351C10P+2
    0xBFFAC1D468DFDE2CU,  // [19] dec: -1.67232171026797530E+000; hex:  -0X1.AC1D468DFDE2C0P+0
    0x3FD5BCD46E19EAF1U   // [20] dec:  3.39650256653540930E-001; hex:   0X1.5BCD46E19EAF10P-2
  };


// the static table of filters
const RP_IIR_FILTER_DESCR iir_hb_lpf_const_filters[] =
{
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  15,                           // order
  100.0000,                     // suppression
  1.5000,                       // ripple
  0.495,                        // left cutoff
  0,                            // right cutoff
  0.0099170047,                 // (??unsure-R&C) transition width
  IIR_LOEL_TYPE0_B,
  IIR_LOEL_TYPE0_A
 }

 ,
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  19,                           // order
  100.0000,                     // suppression
  1.8000,                       // ripple
  0.499,                        // left cutoff
  0,                            // right cutoff
  0.002131305,                  // (??unsure-R&C) transition width
  IIR_LOEL_TYPE1_B,
  IIR_LOEL_TYPE1_A,
 }

 ,
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  18,                           // order
  90.0000,                      // suppression
  2.0000,                       // ripple
  0.499,                        // left cutoff
  0,                            // right cutoff
  0.0015685821,                 // (??unsure-R&C) transition width
  IIR_LOEL_TYPE2_B,
  IIR_LOEL_TYPE2_A,
 }

 ,
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  19,                           // order
  96.0000,                      // suppression
  2.0000,                       // ripple
  0.4982,                       // left cutoff
  0,                            // right cutoff
  0.0015899102,                 // (??unsure-R&C) transition width
  IIR_LOEL_TYPE3_B,
  IIR_LOEL_TYPE3_A,
 }

 ,
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  20,                           // order
  100.0000,                     // suppression
  2.0000,                       // ripple
  0.4982,                       // left cutoff
  0,                            // right cutoff
  0.001414773,                  // (??unsure-R&C) transition width
  IIR_LOEL_TYPE4_B,
  IIR_LOEL_TYPE4_A,
 }

 ,
 {
  RP_IIR_LPF | RP_IIR_ELLIP,
  20,                           // order
  100.0000,                     // suppression
  2.0000,                       // ripple
  0.4985,                       // left cutoff
  0,                            // right cutoff
  0.001414773,                  // (??unsure-R&C) transition width
  IIR_LOEL_TYPE5_B,
  IIR_LOEL_TYPE5_A,
 }

// [...to be continued...]
};


/* filter computation implementation
 * ------ ----------- --------------
 */
/* create the filter by IIR_COEFF
*/
IIR_RAT_POLY *iir_rp_create(const RP_IIR_FILTER_DESCR *fdescr, BOOL is_kahan)
{
 IIR_RAT_POLY *filter = cmalloc(sizeof(IIR_RAT_POLY));
 double a0;
 int i;
 union
 {
  double d;
  uint64_t u;
 } du;                  // we under x86 and only x86

#define U2D(U_)     (du.u = (U_), du.d)

 filter -> pc = filter -> pd = filter -> pz = NULL;
 filter -> nord = fdescr -> order;

 // we suppose, that the new operator always successful
 filter -> pc = cmalloc(filter -> nord * sizeof(double));
 filter -> pd = cmalloc(filter -> nord * sizeof(double));
 filter -> pz = cmalloc(filter -> nord * sizeof(double));

 a0 = U2D(fdescr -> vec_a[0]);
 filter -> d0 = U2D(fdescr -> vec_b[0]) / a0;
 for(i = 0; i < filter -> nord; ++i)
 {
  int j = i + 1;
  filter -> pc[i] = -U2D(fdescr -> vec_a[j]) / a0;
  filter -> pd[i] =  U2D(fdescr -> vec_b[j]) / a0;
 }

 iir_rp_setsum(filter, is_kahan);
 iir_rp_reset(filter);
 return filter;

#undef U2D
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
double iir_rp_process_baseline(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes)
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


/* process one sample -- canonical form / Kahan summations
*/
/* the helpers to compute unstable sums
*/
/* Kahan Summation Formula (X[0] + X[1] + ... + X[N-1]):
 *      S = X[0];
 *      C = 0;
 *      for j = 1 to (N-1) {
 *          Y = X[j] - C
 *          T = S + Y;
 *          C = (T - S) - Y;
 *          S = T;
 *      }
 * ..and if we are brave enogh, we can do hope, that the compiler does not
 * "optimize" it to the "strait" summation..
 */

// summation descriptor -- all the fields as in pseudocode
struct kahan
{
 double S, C, Y, T;
};

// The Kahan summation primitives
// S = X[0]; C = 0;
static __inline void kahan_init(double X0, struct kahan *sum_kh)
{
 sum_kh -> S = X0;
 sum_kh -> C = 0.0;
}
// one step: Y = X[j] - C; Y = X[j] - C; C = (T - S) - Y; 
static __inline void kahan_step(double Xj, struct kahan *sum_kh)
{
 sum_kh -> Y =  Xj          - sum_kh -> C;
 sum_kh -> T =  sum_kh -> S + sum_kh -> Y;
 sum_kh -> C = (sum_kh -> T - sum_kh -> S) - sum_kh -> Y;
 sum_kh -> S =  sum_kh -> T;
}
// one step with floating exceptions checks
static __inline void kahan_step_fes(double Xj, struct kahan *sum_kh, FP_EXCEPT_STATS *fes)
{
#define FES         (fes)

 sum_kh -> Y =     FC(Xj          - sum_kh -> C);
 sum_kh -> T =     FC(sum_kh -> S + sum_kh -> Y);
 sum_kh -> C =  FC(FC(sum_kh -> T - sum_kh -> S) - sum_kh -> Y);
 sum_kh -> S =        sum_kh -> T;

#undef FES
}

// IIR process itself
double iir_rp_process_kahan(double sample, IIR_RAT_POLY *filter, FP_EXCEPT_STATS *fes)
{
#define FES         (fes)

 struct kahan sum_i, sum_o;
 double ti;
 int k = filter -> ix;
 int i;

 kahan_init(sample, &sum_i);

 if(!fes -> is_enable)
 {                                              // == WITHOUT FP CHECKS ==
  // actions for i = 0
  if(0 == k)
   k = filter -> nord;
  --k;

  ti = filter -> pz[k] * filter -> pc[0];
  kahan_step(ti, &sum_i);

  kahan_init(filter -> pz[k] * filter -> pd[0], &sum_o);
  kahan_step(ti              * filter -> d0,    &sum_o);

  // actions for i > 0
  for(i = 1; i < filter -> nord; ++i)
  {
   if(0 == k)
    k = filter -> nord;
   --k;

   ti = filter -> pz[k] * filter -> pc[i];
   kahan_step(ti, &sum_i);

   kahan_step(filter -> pz[k] * filter -> pd[i], &sum_o);
   kahan_step(ti              * filter -> d0,    &sum_o);
  }

  filter -> pz[filter -> ix] = sum_i.S;
  if(++(filter -> ix) >= filter -> nord)        // m_ix = (++m_ix) % m_nord; -- it's BAD for iNTEL
   filter -> ix = 0;

  return sum_o.S;
 }
 else
 {                                              // == WITH FP CHECKS ==
  // actions for i = 0
  if(0 == k)
   k = filter -> nord;
  --k;

  ti = FC(filter -> pz[k] * filter -> pc[0]);
  kahan_step_fes(ti, &sum_i, fes);

  kahan_init    (FC(filter -> pz[k] * filter -> pd[0]), &sum_o);
  kahan_step_fes(FC(ti              * filter -> d0   ), &sum_o, fes);

  // actions for i > 0
  for(i = 1; i < filter -> nord; ++i)
  {
   if(0 == k)
    k = filter -> nord;
   --k;

   ti = FC(filter -> pz[k] * filter -> pc[i]);
   kahan_step_fes(ti, &sum_i, fes);

   kahan_step_fes(FC(filter -> pz[k] * filter -> pd[i]), &sum_o, fes);
   kahan_step_fes(FC(ti              * filter -> d0   ), &sum_o, fes);
  }

  filter -> pz[filter -> ix] = sum_i.S;
  if(++(filter -> ix) >= filter -> nord)        // m_ix = (++m_ix) % m_nord; -- it's BAD for iNTEL
   filter -> ix = 0;

  return sum_o.S;
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

/* change summation algorithm
*/
void iir_rp_setsum(IIR_RAT_POLY *filter, BOOL is_kahan)
{
 filter -> process = is_kahan? &iir_rp_process_kahan : &iir_rp_process_baseline;
}


/* the end...
*/

