/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      in_cwave.h -- input / quad modulation plug-in -- common declarations
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

#if !defined(_in_cwave_h_)
#define _in_cwave_h_

#include "compatibility/compat_gcc.h"

#if !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#if !defined(_CRT_NON_CONFORMING_SWPRINTFS)
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif

#if defined(UNICODE)

#if !defined(UNICODE_INPUT_PLUGIN)      /* for UNICODE we MUST to set UNICODE_INPUT_PLUGIN */
#define UNICODE_INPUT_PLUGIN
#endif

#if !defined(FOPEN_XP)
#define FOPEN_FLG       L"t,ccs=UNICODE"
#else
#define FOPEN_FLG       L"t"            /* for gcc / msvcrt.dll @ XP - w/o support UNICODE stdio */
#endif

#else

#define FOPEN_FLG

#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmreg.h>
#include <tchar.h>
#include <process.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "compatibility/compat_win32_gcc.h"

#include "atomic.h"
#include "cmalloc.h"
#include "fp_check.h"
#include "cwave.h"
#include "hcontrols.h"
#include "crc32.h"
#include "lpf_hilbert_quad.h"
#include "sound_render.h"

#include "../Winamp/in2.h"                      // all of in_ plugin
#include "../Winamp/OUT.H"                      // internal WinAmp renderer API (opt.)

#include "resource.h"

/*
 * Some Definitions
 * ---- -----------
 */

// see OLD_NEWS and CHANGELOG about versioning
#define VERSION_IN_CWAVE        "V2.4.2"

// some compiler specifics
#if defined(_MSC_VER)
#define NOINLINE        __declspec(noinline)
#elif defined(__GNUC__)
#define NOINLINE        __attribute__((noinline))
#else
#error  "Unsupported C compiler"
#endif

#if !defined(MAX_FILE_PATH)
#if defined(UNICODE)
#define MAX_FILE_PATH   (32769)
#else
#define MAX_FILE_PATH   MAX_PATH
#endif
#endif

// max line length in config files
#define MAX_CONFIG_LINE (2048)
#define MAX_CONFIG_KEYW (80)
#define MAX_CONFIG_ARGS (MAX_CONFIG_LINE - MAX_CONFIG_KEYW)

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF  (WM_USER + 2)

#define NS_PERTIME      (576)                   /* samples per time to process */

#define NS_CHECKPT      (8192)                  /* samples per time to CRC check */

#define DEF_PLAY_SLEEP  (20)                    /* default sleep time in playback loop */
#define MAX_PLAY_SLEEP  (100)                   /* max. sleep time in playback loop */

#define MAX_ALIGN_SEC   (20)                    /* max. align time in seconds */

#define MAX_FADE_INOUT  (10000)                 /* max. track fade in / fade out, ms */

// ** we *MISTRUST* for M_PI **
#ifdef  PI
#undef  PI
#endif
#define PI              (3.1415926535897932384626433832795029)

/* Modulator-related types
 * --------- ------- -----
 */
// conversions types
#define S_ADD_REIM      (0)                     /* sum Re(.) + Im(.) */
#define S_SUB_REIM      (1)                     /* sum Re(.) - Im(.) */
#define S_RE            (2)                     /* real output */
#define S_IM            (3)                     /* imaginary output */

// boundary values
#define MAX_FS_SRC      (2000000U)              /* max source sample freqency, Hz; agree with HZ_SCALE  */

#define HZ_SCALE        (1000U)                 /* scaled freq multiplyer, agree with MAX_FS_SRC */

#define MAX_FSHIFT      (20.0)                  /* max shift for one channel, Hz */
#define DEF_FSHIFT      (2.0)                   /* default shift, Hz */

#define MAX_GAIN        (2.0)                   /* max replay gain */
#define DEF_GAIN_MOD    (1.0)                   /* default gain for all excl. Master */
#define DEF_GAIN_MASTER (0.8)                   /* default gain for Master */

#define MAX_PMFREQ      (40.0)                  /* max. PM frequence, Hz */
#define DEF_PMFREQ      (4.0)                   /* default PM frequence, Hz */

#define MIN_PMANGLE     (-1.0)                  /* min. ext. phase angle == -PI */
#define MAX_PMANGLE     (1.0)                   /* max. ext. phase angle == +PI */
#define DEF_PMANGLE     (0.0)                   /* default ext. phase angle */

#define MIN_PMPHASE     (-1.0)                  /* min. int. phase angle == -PI */
#define MAX_PMPHASE     (1.0)                   /* max. int. phase angle == +PI */
#define DEF_PMPHASE     (0.0)                   /* default int. phase angle */

#define MAX_PMLEVEL     (1.0)                   /* max. mod. level == PI */
#define DEF_PMLEVEL     (0.5)                   /* default phase mod. level */

// channel exchange modes
#define XCH_NORMAL      (0)                     /* leave unchanged */
#define XCH_SWAP        (1)                     /* swap left and right */
#define XCH_LEFTONLY    (2)                     /* both == left */
#define XCH_RIGHTONLY   (3)                     /* both == right */
#define XCH_MIXLR       (4)                     /* both == (left + right) / 2 */

#define XCH_DEF         (XCH_NORMAL)            /* default value */
#define XCH_MAX         (4)                     /* max. for config etc. */

/* Data Types
 * ---- -----
 */
#define N_INPUTS        (1+26)                  /* number of inputs - in[0],A[1]..Z[26] */
#define MAX_DSP_NAME    (80)                    /* max length of node name */
#define SIZE_DSP_NAME   (MAX_DSP_NAME + 16)     /* full size of name buffer in a node */

// partial DSP-processing types
// ------- -------------- -----
// "Master" output
typedef struct tagCMAKE_MASTER                  // one channel specific for master
{
 volatile int tout;                             // type of the output [S_RE/S_IM/S_SUM_REIM]
} CMAKE_MASTER;

typedef struct tagMAKE_MASTER                   // full "master" type
{
 CMAKE_MASTER le;                               // left output
 CMAKE_MASTER ri;                               // right output
} MAKE_MASTER;

// "Shift" output
typedef struct tagCMAKE_SHIFT                   // one channel specific for shift
{
 volatile double fr_shift;                      // shift for left with sign [-MAX_FSHIFT..MAX_FSHIFT]
 volatile int is_shift;                         // 0 - channel unchanged (bypass)
} CMAKE_SHIFT;

typedef struct tagMAKE_SHIFT                    // full "shift" type
{
 CMAKE_SHIFT le;                                // left output
 CMAKE_SHIFT ri;                                // right output
 volatile int n_out;                            // output plug [1..N_INPUTS-1]
 int lock_shift;                                // 1 == L/R control locked
 int sign_lock_shift;                           // 1 == L/R controls "mirrored"
} MAKE_SHIFT;

// "PM" output
typedef struct tagCMAKE_PM                      // one channel specific for PM
{
 volatile double freq;                          // PM frequency [0..MAX_PMFREQ]
 volatile double phase;                         // PM "internal" phase [MIN_PMPHASE..MAX_PMPHASE]
 volatile double level;                         // PM level [0..MAX_PMLEVEL]
 volatile double angle;                         // initial phase [MIN_PMANGLE..MAX_PMANGLE]
 volatile int is_pm;                            // 0 - channel unchanged (bypass)
} CMAKE_PM;

typedef struct tagMAKE_PM                       // full "PM" type
{
 CMAKE_PM le;                                   // left output
 CMAKE_PM ri;                                   // right output
 volatile int n_out;                            // output plug [1..N_INPUTS-1]
 int lock_freq;                                 // 1 == L/R control locked
 int lock_phase;                                // 1 == L/R control locked
 int lock_level;                                // 1 == L/R control locked
 int lock_angle;                                // 1 == L/R control locked
} MAKE_PM;

// "Mix" output
typedef struct tagMAKE_MIX
{
 volatile int n_out;                            // output plug [1..N_INPUTS-1]
} MAKE_MIX;

/* all types of outputs
*/
typedef union tagMAKE_DSP
{
 MAKE_MASTER mk_master;
 MAKE_SHIFT mk_shift;
 MAKE_PM mk_pm;
 MAKE_MIX mk_mix;
} MAKE_DSP;

/* node list or put them together
*/
typedef struct tagNODE_DSP
{
 struct tagNODE_DSP *prev;                      // link to previous node
 struct tagNODE_DSP *next;                      // link to next node
 volatile double l_gain;                        // left gain [0..MAX_GAIN]
 volatile double r_gain;                        // right gain [0..MAX_GAIN]
 char inputs[N_INPUTS];                         // inputs to mix (bool)
 int xch_mode;                                  // channels exchange mode XCH_xxx
 int l_iq_invert;                               // left ch. inversion of spectrum (I/Q swap), bool
 int r_iq_invert;                               // right ch. inversion of spectrum (I/Q swap), bool
 int mode;                                      // the type of MAKE_DSP
 MAKE_DSP dsp;                                  // DSP specific data according mode
 TCHAR name[SIZE_DSP_NAME];                     // readable name of DSP node
 int lock_gain;                                 // 1 == L/R control locked
} NODE_DSP;
// .mode can get the next values
#define MODE_MASTER     (0)                     /* master */
#define MODE_SHIFT      (1)                     /* shift */
#define MODE_PM         (2)                     /* phase modulation */
#define MODE_MIX        (3)                     /* mix -- all of .dsp ignored */
// illegal value for NODE_DSP *:
#define INVALID_NODE_DSP        ((NODE_DSP *)((INT_PTR)(-1)))

/* the complex type
*/
typedef struct tagCCOMPLEX
{
 double re;                                     // real part (I/In phase subchannel)
 double im;                                     // imaginary part (Q/Quadrature subchannel)
} CCOMPLEX;

/* input/output cell
*/
typedef struct tagLRCOMPLEX
{
 CCOMPLEX le;                                   // left channel
 CCOMPLEX ri;                                   // right channel
} LRCOMPLEX;

/* xwave-reader related stuff
 * ----- ------ ------- -----
 */
#define MIN_FILE_SAMPLES        (2)             /* minimum samples in a file */

/* definitions for RWAVE-WAV stuff
*/
// file types
#define XW_TYPE_UNKNOWN         (~0)            /* input file type unknown */
#define XW_TYPE_CWAVE           (0)             /* input file type CWAVE */
#define XW_TYPE_RWAVE           (1)             /* input file type RWAVE (WAV) */

// supported WAV sample formats
#define HRW_FMT_UNKNOWN         (~0)            /* unknown format */
#define HRW_FMT_UINT8           (0)             /* unsigned 8 bits */
#define HRW_FMT_INT16           (1)             /* signed 16 bits */
#define HRW_FMT_INT24           (2)             /* signed 24 bits */
#define HRW_FMT_INT32           (3)             /* signed 32 bits */
#define HRW_FMT_FLOAT32         (4)             /* IEEE754 folat, (-1.0..1.0) */

// supported WAV header types
#define HRW_HTYPE_WFONLY        (0)             /* WAVFORMAT ONLY (bad!) */
#define HRW_HTYPE_PCMW          (1)             /* PCMWAVEFORMAT, incl. float */
#define HRW_HTYPE_EXT           (2)             /* WAVEFORMATEXTENSIBLE, incl. float */

/* the internal types
*/
// the function - sample converter of _complex_ raw data to normalized to [-32768.0..32767] complex
typedef void (*UNPACK_IQ)(double *vI, double *vQ, const BYTE **buf);
// the function - sample converter of _pure real_ raw data to normalized to [-32768.0..32767] real
typedef void (*UNPACK_RE)(double *vR, const BYTE **buf);

// universal unpack sample converter
typedef union tagUNPACK_SAMPLE
{
 UNPACK_IQ unpack_iq;
 UNPACK_RE unpack_re;
} UNPACK_SAMPLE;

// CWAVE-only specifics
typedef struct tagXCWAVE
{
 HCWAVE header;                                 // the heder as is
 TMP_CRC32 cur_crc;                             // CRC32 of the current file (if need)
} XCWAVE;

// RWAVE (WAV) only specifics
typedef struct tagXRWAVE
{
 unsigned format;                               // sample format, HRW_FMT_xxx
 unsigned htype;                                // the header type HRW_HTYPE_xxx
 WAVEFORMATEXTENSIBLE header;                   // the header, according htype
} XRWAVE;

// CWAVE+RWAVE=XWAVE descriptor
typedef union tagXWAVE
{
 XCWAVE cwave;
 XRWAVE rwave;
} XWAVE;

/* the full reader descriptor
*/
typedef struct tagXWAVE_READER
{
 TCHAR *file_full_name;                         // full name of the file
 TCHAR *file_path;                              // the path to the file with '\\' at the end
 TCHAR *file_pure_name;                         // name of the file w/o path
 HANDLE file_hanle;                             // the WIN32 file handle

 unsigned type;                                 // type as XW_TYPE_xxx
 XWAVE spec;                                    // type specific
 BOOL is_sample_complex;                        // FALSE == pure real samples
 UNPACK_SAMPLE unpack_handler;                  // sample unpacker @ .is_sample_complex

 unsigned n_channels;                           // number of channels
 int64_t n_samples;                             // number of samples in file
 int64_t n_tail;                                // number of samples in virtual silence tail
 unsigned sample_rate;                          // sample rate of the source
 unsigned ch_sample_size;                       // single channel sample size
 unsigned sample_size;                          // size of n_channel frame of samples
 int64_t offset_data;                           // offset of audio data in the file
 int64_t pos_samples;                           // current position in file in samples
 int64_t pos_tail;                              // position in virtual silence tail
 int64_t n_fade_in;                             // fade in, samples
 int64_t n_fade_out;                            // fade_out, samples

 BYTE *tbuff;                                   // the data buffer for read quant
 BYTE *ptr_tbuff;                               // pointer to sample-based reader
 BYTE *zero_sample;                             // pointer to zero (silence) sample
 unsigned read_quant;                           // quant of the data to read, samples
 unsigned really_readed;                        // really readed after last read, samples
 unsigned unpacked;                             // already unpacked (in mean "unpacked to double")
} XWAVE_READER;

/* the modulator context for the conversion thread / interface (playback and transcode)
 * --- --------- ------- --- --- ---------- ------ - --------- --------- --- ----------
 */
typedef struct tagMOD_CONTEXT
{
 CRITICAL_SECTION cs_n_frame;                   // sample counter protector
 volatile uint64_t n_frame;                     // sample counter
 LRCOMPLEX inout[N_INPUTS];                     // in/out bus
 LPF_HILBERT_QUAD *h_left;                      // analytic transformer-left channel
 LPF_HILBERT_QUAD *h_right;                     // analytic transformer-right channel
 XWAVE_READER *xr;                              // current reader
 SOUND_RENDER sr_left;                          // sound render-left channel
 SOUND_RENDER sr_right;                         // sound render-right channel
 FP_EXCEPT_STATS fes_hilb_left;                 // exception statistics for analitic transformer-left channel
 FP_EXCEPT_STATS fes_hilb_right;                // exception statistics for analitic transformer-right channel
 FP_EXCEPT_STATS fes_sr_left;                   // exception statistics for sound render-left channel
 FP_EXCEPT_STATS fes_sr_right;                  // exception statistics for sound render-right channel
} MOD_CONTEXT;

/* The Our All -- plugin-wide variables
 * --- --- --- -- ----------- ---------
 */
// values of infobox_parenting:
#define INFOBOX_NOPARENT        (0)             /* infobox has no parent */
#define INFOBOX_LISTPARENT      (1)             /* infobox has playlist as parent */
#define INFOBOX_MAINPARENT      (2)             /* infobox has main player's window as parent */

// helper - the struct to contain variables for config
typedef struct tagIN_CWAVE_CFG
{
 unsigned ver_config;                           // config file version, > 0
 BOOL is_wav_support;                           // true, if the module support WAV
 BOOL is_rwave_support;                         // true, if the module support RWAVE ext. for WAV
 unsigned infobox_parenting;                    // value INFOBOX_xxx - behaviour of "Alt-3" window
 BOOL enable_unload_cleanup;                    // enable cleanup on unload plugin
 unsigned play_sleep;                           // sleep while playback, ms
 BOOL disable_play_sleep;                       // disable sleep on plyback
 unsigned sec_align;                            // time in seconds to align file length
 unsigned fade_in;                              // track fade in, ms
 unsigned fade_out;                             // track fade out, ms
 BOOL is_frmod_scaled;                          // true, if unsigned scaled modulation frequencies in use
 unsigned iir_filter_no;                        // current HB LPF for quad Hilbert conv. -- IX_LPF_HILB_xxx
 IIR_COMP_CONFIG iir_comp_config;               // IIR HB LPF computation config
 BOOL is_clr_nframe_trk;                        // clear sample counter per each track
 BOOL is_clr_hilb_trk;                          // clear analitic transformers per track
 BOOL show_long_numbers;                        // show reasonable many digits in operating parameters
 BOOL is_fp_check;                              // true, if floating point check set
 // sound render part
 BOOL need24bits;                               // FALSE -- 16 bits decoding (real output of playback/transcode)
 SR_VCONFIG sr_config;                          // all of volatile sound renders parameters
// the copy last dsp-list
 NODE_DSP *dsp_list;                            // temporary copy for to/from modulator transfers
} IN_CWAVE_CFG;

// ..and here is it:
typedef struct tagIN_CWAVE
{
// the configurable part:
 IN_CWAVE_CFG cfg;                              // filled in config.c
 TCHAR *def_conf_filename;                      // filled by load_config_default();
                                                // free by save_config_default()
 CRITICAL_SECTION cs_hilbert_change;            // Access to Hilbert converters internals
 CRITICAL_SECTION cs_render_change;             // lock _for_ all sound renders
 CRITICAL_SECTION cs_playback_mc_access;        // lock for using mc_playback non-permanent object(s),
                                                // e.g. XWAVE_READER
 // modulator conexts (include analitic converters)
 MOD_CONTEXT mc_playback;                       // modulator context for playback
 MOD_CONTEXT mc_transcode;                      // modulator context for transcode
} IN_CWAVE;


/* The general stuff/config (in_cwave.c)
 * --- ------- ------------ ------------
 */
// Globals: The Our All
extern IN_CWAVE the;                            // The Our All
/* lock Hilberts transformers from change
*/
void all_hilberts_lock(void);
/* unlock Hilberts transformers to change
*/
void all_hilberts_unlock(void);
/* reset the modulator-context's Hilbert's transformer
*/
void mod_context_reset_hilbert(MOD_CONTEXT *mc);
/* change computation rules for all IIR-filters in all Hilbert transformers
*/
void mod_context_change_all_hilberts_config(const IIR_COMP_CONFIG *new_comp_cfg);
/* change the current Hilbert's transformers
*/
void mod_context_change_all_hilberts_filter(unsigned new_filter_no);
/* lock Hilberts transformers from change
*/
void all_hilberts_lock(void);
/* unlock Hilberts transformers to change
*/
void all_hilberts_unlock(void);
/* open a file to processing with a MOD_CONTEXT object
*/
BOOL mod_context_fopen(const TCHAR *name, unsigned read_quant,  // for XWAVE_READER
        MOD_CONTEXT *mc);
/* close a file was processing with a MOD_CONTEXT object
*/
void mod_context_fclose(MOD_CONTEXT *mc);
/* clear (unused) in-out in the all module-contexts
*/
void mod_context_clear_all_inouts(int nclr);
/* lock frame counter
*/
void mod_context_lock_framecnt(MOD_CONTEXT *mc);
/* unlock frame counter
*/
void mod_context_unlock_framecnt(MOD_CONTEXT *mc);
/* get value of frame counter
*/
uint64_t mod_context_get_framecnt(MOD_CONTEXT *mc);
/* clear frame counter
*/
void mod_context_reset_framecnt(MOD_CONTEXT *mc);
/* get value of desubnorm protection counter
*/
uint64_t mod_context_get_desubnorm_counter(MOD_CONTEXT *mc);
/* reset desubnorm protection counter
*/
void mod_context_reset_desubnorm_counter(MOD_CONTEXT *mc);
/* set new mode for the all FP exception counters
*/
void fecs_set_endis_all(BOOL new_endis);
/* reset float excetions counters by group
*/
void fecs_resets(BOOL res_hilb_le, BOOL res_hilb_ri, BOOL res_sr_le, BOOL res_sr_ri);
/* get float exceptions statistics by group
*/
void fecs_getcnts(
      FP_EXCEPT_STATS *fec_hilb_le
    , FP_EXCEPT_STATS *fec_hilb_ri
    , FP_EXCEPT_STATS *fec_sr_le
    , FP_EXCEPT_STATS *fec_sr_ri
    );
/* get current _volatile config_ sound render's parameters
*/
void srenders_get_vcfg(SR_VCONFIG *sr_cfg);
/* set _volatile config_ for _all_ sound renders
*/
void srenders_set_vcfg(const SR_VCONFIG *sr_cfg);
/* lock access to render's volatile objects
*/
void all_srenders_lock(void);
/* unlock access to render volatile objects
*/
void all_srenders_unlock(void);
/* lock mc_playback access to it's non-permanent objects
*/
void mp_playback_lock(void);
/* unlock mc_playback access to it's non-permanent objects
*/
void mp_playback_unlock(void);
/* sleep for playback according config
*/
void playback_sleep(void);
/* exported symbol -- read config; returns module interface
*/
__declspec(dllexport) In_Module *winampGetInModule2(void);
/* the whole module cleanup -- complemetn to winampGetInModule2()
*/
void module_cleanup(void);

/* the load / save config module
 * --- ---- - ---- ------ ------
 */
/* load the config by file name (NULL - default)
*/
BOOL load_config(const TCHAR *config_name);
/* save the config by the file name (NULL - default)
*/
BOOL save_config(const TCHAR *config_name);
/* load default config
*/
BOOL load_config_default(void);
/* save default config
*/
BOOL save_config_default(void);

/* The playback and it's service (playback.c)
 * --- -------- --- ---- ------- ------------
 */
/* show setup by ALT+3
*/
BOOL getShowPlay(void);
void setShowPlay(BOOL nsp);
/* Return In_Module interface
*/
In_Module *get_playback_iface(void);
/* make the .FileExtensions list for playback interface
*/
void make_exts_list(BOOL is_wav, BOOL is_rwave);

/* advanced modulator (adv_modulator.c)
 * -------- --------- -----------------
 */
/* one-time initialization of advanced modulator
*/
int amod_init(NODE_DSP *cfg_list, BOOL is_frmod_scaled);        // !0, if list accepted
/* cleanup advanced modulator (+, probably transfer DSP-list)
*/
NODE_DSP *amod_cleanup(int is_transfer);
/* delete the whole DSP list
*/
void amod_del_dsplist(void);
/* delete the last DSP list element
*/
void amod_del_lastdsp(void);
/* add the last element to the DSP list
*/
NODE_DSP *amod_add_lastdsp(const TCHAR *name, int mode);
/* get the state of bypass list flag
*/
BOOL amod_get_bypass_list_flag(void);
/* set the state of bypass list flag
*/
void amod_set_bypass_list_flag(BOOL bypass);
/* get DSP list head for keep-the-structure operation
*/
NODE_DSP *amod_get_headdsp(void);
/* set DSP node outplug -- thread safe
*/
void amod_set_output_plug(NODE_DSP *ndEd, int n);               // n == -1 -> remove only
/* get channel's clips counters and peak values
*/
void amod_get_clips_peaks
    ( unsigned *lc
    , unsigned *rc
    , double *lpv
    , double *rpv
    , BOOL isReset
    );
/* convert a frequency to it's "true" value
*/
double amod_true_freq(double raw_freq);
/* process XWAVE_READER.really_readed samples into a bufffer
*/
int amod_process_samples(char *buf, MOD_CONTEXT *mc);

/* xwave-reader interface (xwave_reader.c)
 * ------------ --------- ----------------
 */
/* create reader -- one reader per file; return NULL if error
*/
XWAVE_READER *xwave_reader_create(
      const TCHAR *name
    , unsigned read_quant
    , unsigned sec_align
    , unsigned fade_in
    , unsigned fade_out
    );
/* destroy the reader
*/
void xwave_reader_destroy(XWAVE_READER *xr);
/* get total number of samples in xwave_reader object (real + virtual)
*/
int64_t xwave_get_nsamples(XWAVE_READER *xr);
/* abs seek in samples terms in the file (non-unpacked data will be lost)
*/
BOOL xwave_seek_samples(int64_t sample_pos, XWAVE_READER *xr);  // FALSE == bad
/* abs seek in miliseconds terms in the file (non-unpacked data will be lost)
*/
BOOL xwave_seek_ms(int64_t ms_pos, XWAVE_READER *xr);   // FALSE == bad
/* change read quant (non-unpacked data will be lost)
*/
void xwave_change_read_quant(unsigned new_rq, XWAVE_READER *xr);
/* read the next portion of data (tbuff[read_quant])
*/
BOOL xwave_read_samples(XWAVE_READER *xr);
/* unpack one sample as complex for left and right channels (need external lock for Hilb. conv.)
*/
void xwave_unpack_csample(double *lI, double *lQ, double *rI, double *rQ,
        MOD_CONTEXT *mc, XWAVE_READER *xr);

/* GUI module (gui_cwave.c)
 * --- ------ -------------
 */
/* one-time module initialization
*/
BOOL gui_init(void);
/* module cleanup
*/
void gui_cleanup(void);
/* keep window(s) alive while GUI thread busy
*/
void make_events(void);
/* the front-end for file info dialog
*/
void gui_info_dialog(HINSTANCE hi, HWND hwndParent, XWAVE_READER *xr);

/* Advanced modulator GUI control (amod_gui_control.c)
 * -------- --------- --- ------- --------------------
 */
/* front-end advanced modulator GUI setup
*/
void amgui_init(void);
/* front-end advanced modulator GUI cleanup
*/
void amgui_cleanup(void);
/* the front-end for advanced modulator GUI setup dialog
*/
void amgui_setup_dialog(HINSTANCE hi, HWND hwndParent);

#endif                                                  // def _in_cwave_h_

/* the end...
*/

