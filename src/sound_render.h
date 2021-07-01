/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      sound_render.h -- all about convertig double to the integer sound samples
 *
 * Copyright (c) 2010-2021, Rat and Catcher Technologies
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

#if !defined(_sound_render_h_)
#define _sound_render_h_

#include "compatibility/compat_gcc.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <windows.h>
#include <math.h>
#include <tchar.h>

#include "fp_check.h"
#include "mersene_twister/mt_jrnd.h"

#include "compatibility/compat_win32_gcc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* The type(s) of quantize (rounding to int)
*/
#define SND_QUANTZ_MID_TREAD    (0)                     /* simple rounding (-0.5..0.5) == 0 */
#define SND_QUANTZ_MID_RISER    (1)                     /* [0..1) == 0 -- recomended */

#define SND_QUANTZ_MAX          (1)                     /* max of value SND_QUANTZ_xxx */

/* The type(s) of the rendering
*/
#define SND_RENDER_ROUND        (0)                     /* simple round */
#define SND_RENDER_RPDF         (1)                     /* rectanglular PDF dithering */
#define SND_RENDER_TPDF         (2)                     /* triangular PDF dithering */
#define SND_RENDER_STPDF        (3)                     /* "sloped" triangular PDF dithering */
#define SND_RENDER_GAUSS        (4)                     /* [pseudo?]Gaussian dithering */

#define SND_RENDER_MAX          (4)                     /* max. value of SND_RENDER_xxx */

/* The type(s) on noise shaping
*/
#define SND_NSHAPE_FLAT         (0)                     /* none ("flat") noise shaping */
#define SND_NSHAPE_FW44         (1)                     /* 44100 Hz F-weighted */
#define SND_NSHAPE_MEW44        (2)                     /* 44100/48000 Hz modified E-weighted */
#define SND_NSHAPE_IEW44        (3)                     /* 44100/48000 Hz improved E-weighted */
#define SND_NSHAPE_LIPSH44      (4)                     /* 44100 Hz Lipshitz */
#define SND_NSHAPE_SHIB48       (5)                     /* 48000 Hz Shibata */
#define SND_NSHAPE_SHIB44       (6)                     /* 44100 Hz Shibata */
#define SND_NSHAPE_SHIB38       (7)                     /* 38000 Hz Shibata */
#define SND_NSHAPE_SHIB32       (8)                     /* 32000 Hz Shibata */
#define SND_NSHAPE_SHIB22       (9)                     /* 22050 Hz Shibata */
#define SND_NSHAPE_SHIB16       (10)                    /* 16000 Hz Shibata */
#define SND_NSHAPE_SHIB11       (11)                    /* 11025 Hz Shibata */
#define SND_NSHAPE_SHIB8        (12)                    /* 8000 Hz Shibata */
#define SND_NSHAPE_LOSHIB48     (13)                    /* 48000 Hz Low Shibata */
#define SND_NSHAPE_LOSHIB44     (14)                    /* 44100 Hz Low Shibata */
#define SND_NSHAPE_HISHIB44     (15)                    /* 44100 Hz High Shibata */
#define SND_NSHAPE_GES44        (16)                    /* 44100 Hz -- Gesemann */
#define SND_NSHAPE_GES48        (17)                    /* 48000 Hz -- Gesemann */

#define SND_NSHAPE_MAX          (17)                    /* max. value of SND_NSHAPE_xxx */

/* default / max bits to dither
*/
#define DEF_DITHER_BITS         (1.0)                   /* +-LSBit(s) */
#define MAX_DITHER_BITS         (23.0)                  /* max for 24bits sample; ugly for 16 bits */

/* misc
*/
#define ZERO_SIGNAL_DB          (-220.0)                /* zero signal value in dB */

/* the noise shaping dithering type
 * --- ----- ------- --------- ----
 */
// forward type(s)
typedef struct tagNS_SHAPER NS_SHAPER;

// the corresponding shaping filter
typedef double (*NS_SHAPER_FILTER)(double val, NS_SHAPER *nss, FP_EXCEPT_STATS *fes);

// the shaper description
typedef struct tagNS_SHAPER_DSC
{
 NS_SHAPER_FILTER ns_filter;                            // the current noise shaper
 const double *filter_coeffs;                           // current coefficients for current shaper
 unsigned num_coeffs;                                   // number of filter coefficients (filter order for IIR)
 unsigned target_sample_rate;                           // nominal sample rate for the filter, 0==any
} NS_SHAPER_DSC;

// the shaper in whole
struct tagNS_SHAPER
{
 const NS_SHAPER_DSC *dsc;                              // current noise shaper descriptor
 double *ns_ebuffer;                                    // the filter signal bufer ("errors")
 double *ns_obuffer;                                    // additional IIR-only buffer ("outputs")
 int ns_ix_pos;                                         // the last position, where we have placed a signal
};

/* the "volatile parameters" of the sound render (wich can be changed on the fly)
*/
typedef struct tagSR_VCONFIG
{
 double dth_bits;                                       // +-LSB in output bits for dithering
 unsigned quantz_type;                                  // SND_QUANTZ_xxx
 unsigned render_type;                                  // SND_RENDER_xxx
 unsigned nshape_type;                                  // SND_NSHAPE_xxx
} SR_VCONFIG;

/* the render object
*/
typedef struct tagSOUND_RENDER
{
 volatile SR_VCONFIG config;                            // to change in real time
 double dth_mul;                                        // dithering multiplyer = 2**dth_bits - 1
 double prev_rnd;                                       // previous (-1..+1) random (for sloped TPDF only)
 double hi_bound;                                       // high bound (always less)
 double lo_bound;                                       // low bound (alvays greater)
 double norm_mul;                                       // 1.0==16 bits, 256.0==24 bits
 double round_offset;                                   // 0.5 to mid tread, 0 to mid riser
 double prev_ns_err;                                    // previous noise shaping error correction
 int sign_delta;                                        // to _ADD_ to int result, if input < 0
 int is24bits;                                          // ==0 - output to 16 bits; else -- to 24
 NS_SHAPER ns_shaper;                                   // our shaper
 MT_JRND_STATE jrnd;                                    // current state of random numbers gen
} SOUND_RENDER;

/* the interface
 * --- ---------
 */
/* one-time init the render with the random's seed
*/
void sound_render_init(const SR_VCONFIG *cfg,   int need24bits, uint32_t seed, SOUND_RENDER *sr);
/* sound render deinitialization
*/
void sound_render_cleanup(SOUND_RENDER *sr);
/* set sound render to 16/24 output bits -- to be call in safe places (beetween files) only
*/
void sound_render_set_outbits(int need24bits, SOUND_RENDER *sr);
/* setup the render with new config -- exclude 16/24 bits
*/
void sound_render_setup(const SR_VCONFIG *cfg, SOUND_RENDER *sr);
/* copy one sound render configuration to another
*/
void sound_render_copy_cfg(const SR_VCONFIG *src, volatile SR_VCONFIG *dst);
/* return string list ordered according .quantz_type indexes
*/
const TCHAR **sound_render_get_quantznames(void);
/* return string list ordered according .render_type indexes
*/
const TCHAR **sound_render_get_rtypenames(void);
/* return string list ordered according .nshape_type indexes
*/
const TCHAR **sound_render_get_nshapenames(void);
/* get the length of output sample (each sound render is one channel only!)
*/
unsigned sound_render_size(SOUND_RENDER *sr);
/* render a sample value to buffer (**buf not unsigned due to WinAmp conventions)
*/
void sound_render_value
    ( char **buf
    , double input
    , unsigned *clip_cnt
    , double *peak_val
    , SOUND_RENDER *sr
    , FP_EXCEPT_STATS *fes
    );
/* return level in dB for zero-valued signal
*/
double sound_render_get_zero_db(void);


#if defined(__cplusplus)
}
#endif

#endif          // def _sound_render_h_

/* the end...
*/

