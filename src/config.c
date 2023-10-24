/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      config.c -- load / save module configuration; add-on to in_cwave.c
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

#include "in_cwave.h"

#include <shlobj.h>
#include <stdio.h>

/* TAKE CARE OF:
 * ---- ---- ---
 * In all cases name of parameter is less that 80 symbols. Other part of the
 * config string is less, than MAX_CONFIG_ARGS symbols. Total length of config
 * string is less that MAX_CONFIG_LINE symbols. This is hardcoded below.
 * It's rather simple and rather safe.
 */

/* DO NOT FORGET:
 * -- --- ------
 * !CHANGE VERSION_CONFIG EVERY TIME WHEN CONFIG FILE FORMAT CHANGED!
 */

#define VERSION_CONFIG                  (9U)    /* we start to count cfg versions from 1 */

/* NOTE: It's a simple config handler -- this is not a beauty language. If anybody
 * like to write / edit configs -- the way is clear, but silly -- there is no way to set
 * the something, that can't be set from plugin setup window(s). Just in case --
 * note about ' ' and '%' processing in the string fields (see handle_string()).
 */

/* the types of variables to store/read config
*/
#define VCFG_FUNCTION                   (0)     /* need function to read */
#define VCFG_BOOL                       (1)     /* 0 or 1*/
#define VCFG_INT                        (2)     /* C-lang int */
#define VCFG_UNSIGNED                   (3)     /* C-lang unsigned */
#define VCFG_DOUBLE                     (4)     /* C-lang double as is */
#define VCFG_DOUBLE_BIN                 (5)     /* C-lang double saved as 0x<hex-nibbles>ULL */
#define VCFG_STRING                     (6)     /* string, see below */

/* the default/bound values for different types
*/
typedef struct tagDB_VAUES_INT                  // for VCFG_INT
{
 int v_def;
 int v_min;
 int v_max;
} DB_VAUES_INT;
typedef struct tagDB_VAUES_UNSIGNED             // for VCFG_UNSIGNED
{
 unsigned v_def;
 unsigned v_min;
 unsigned v_max;
} DB_VAUES_UNSIGNED;
typedef struct tagDB_VAUES_DOUBLE               // for VCFG_DOUBLE / VCFG_DOUBLE_WLL
{
 double v_def;
 double v_min;
 double v_max;
} DB_VAUES_DOUBLE;

/* type of the function to read / write config node;
 * if buf == NULL && !toWrite -- simple set target value to default
 */
// forward type(s)
typedef struct tagCONFIG CONFIG;

// the function type
typedef BOOL (*SPEC_CFG_RW)(FILE *fp, TCHAR **buf,
        const CONFIG *cfg_node, BOOL toWrite, BOOL is_empty);

/* the config node itself
*/
struct tagCONFIG
{
 const TCHAR *name;                     // the name of parameter
 const unsigned type;                   // type - VCFG_xxx
 volatile void *val;                    // pointer to value
 const void *db_or_fun;                 // pointer to DB_VAUES_xxx/single default value or SPEC_CFG_RW *
};

/* forwards to possible (*SPEC_CFG_RW)()
*/
static BOOL handle_node_dsp(FILE *fp, TCHAR **buf,
        const CONFIG *cfg_node, BOOL toWrite, BOOL is_empty);

/* the module default / bounds value according IN_CWAVE_CFG
 * --- ------ ------- - ------ ----- --------- ------------
 */
// unsigned ver_config; config file version, > 0
static const DB_VAUES_UNSIGNED DB_ver_config =
{ 0, 0, ~0 /* to special check */ };

// BOOL is_wav_support; true, if the module support WAV
static const BOOL DB_is_wav_support = TRUE;

// BOOL is_rwave_support; true, if the module support RWAVE ext. for WAV
static const BOOL DB_is_rwave_support = FALSE;

// unsigned infobox_parenting; value INFOBOX_xxx - behaviour of "Alt-3" window
static const DB_VAUES_UNSIGNED DB_infobox_parenting =
{ INFOBOX_LISTPARENT, INFOBOX_NOPARENT, INFOBOX_MAINPARENT };

// BOOL enable_unload_cleanup; enable cleanup on unload plugin
static const BOOL DB_enable_unload_cleanup = FALSE;

// unsigned play_sleep; sleep while playback, ms
static const DB_VAUES_UNSIGNED DB_play_sleep =
{ DEF_PLAY_SLEEP, 0, MAX_PLAY_SLEEP };

// BOOL disable_play_sleep; disable sleep on plyback
static const BOOL DB_disable_play_sleep = FALSE;

// unsigned sec_align; time in seconds to align file length
static const DB_VAUES_UNSIGNED DB_sec_align =
{ 0, 0, MAX_ALIGN_SEC };

// unsigned fade_in; track fade in, ms
static const DB_VAUES_UNSIGNED DB_fade_in =
{ 0, 0, MAX_FADE_INOUT };

// unsigned fade_out; track fade out, ms
static const DB_VAUES_UNSIGNED DB_fade_out =
{ 0, 0, MAX_FADE_INOUT };

// BOOL is_frmod_scaled; true, if unsigned scaled modulation frequencies in use
static const BOOL DB_is_frmod_scaled = TRUE;

// unsigned iir_filter_no; number of current HB LPF for quad
static const DB_VAUES_UNSIGNED DB_iir_filter_no =
{ IX_IIR_LOEL_DEF, IX_IIR_LOEL_TYPE0, IX_IIR_LOEL_TYPEMAX };

// BOOL iir_comp_config.is_kahan; type of process IIR computation (Kahan / baseline summation)
static const BOOL DB_iir_sum_kahan = TRUE;

// BOOL iir_comp_config.is_subnorm_reject; suppressing low values in "delay line" of IIR state
static const BOOL DB_iir_subnorm_reject = TRUE;

// double iir_comp_config.subnorm_thr; subnorm reject threshold
static const DB_VAUES_DOUBLE DB_iir_subnorm_thr =
{ SBN_THR_DEF, SBN_THR_MIN, SBN_THR_MAX };

// BOOL is_clr_nframe_trk; TRUE when we want to clear frame counter per each track
static const BOOL DB_is_clr_nframe_trk = FALSE;

// BOOL is_clr_hilb_trk; TRUE when we want to clear analitic transformers per each track
static const BOOL DB_is_clr_hilb_trk = FALSE;

// BOOL show_long_numbers; FALSE -- short format, TRUE -- %G or something same
static const BOOL DB_show_long_numbers = FALSE;

// BOOL is_fp_check; true, if floating point check set
static const BOOL DB_is_fp_check = FALSE;

// BOOL is24bits; FALSE -- 16 bits decoding (real output of playback/transcode)
static const BOOL DB_need24bits = TRUE;

// double sr_config.dth_bits; bits to dither (+-LSbits; _TPDF reduced_)
static const DB_VAUES_DOUBLE DB_dth_bits =
{ DEF_DITHER_BITS, 0.0, MAX_DITHER_BITS };

// unsigned sr_config.quantz_type; type of rounding while convert double to int
static const DB_VAUES_UNSIGNED DB_quantz_type =
{ SND_QUANTZ_MID_RISER, SND_QUANTZ_MID_TREAD, SND_QUANTZ_MAX };

// unsigned sr_config.render_type; type of dithering (rendering) while convert double to int
static const DB_VAUES_UNSIGNED DB_render_type =
{ SND_RENDER_ROUND, SND_RENDER_ROUND, SND_RENDER_MAX };

// unsigned sr_config.nshape_type; noise shaping type SND_NSHAPE_xxx
static const DB_VAUES_UNSIGNED DB_nshape_type =
{ SND_NSHAPE_FLAT, SND_NSHAPE_FLAT, SND_NSHAPE_MAX };

// unsigned sr_config.sign_bits16; significant bits for 16bit output
static const DB_VAUES_UNSIGNED DB_sign_bits16 =
{ DEF_SIGN_BITS16, MIN_SIGN_BITS16, MAX_SIGN_BITS16 };

// unsigned sr_config.sign_bits24; significant bits for 24bit output
static const DB_VAUES_UNSIGNED DB_sign_bits24 =
{ DEF_SIGN_BITS24, MIN_SIGN_BITS24, MAX_SIGN_BITS24 };

// [NOT-NEED-HERE] NODE_DSP *dsp_list; the copy last dsp-list

/* the full list of the module configuration
 * --- ---- ---- -- --- ------ -------------
 */
static const CONFIG config_list[] =
{
// unsigned ver_config; config file version, > 0
// ** THIS ALWAYS HAS INDEX == 0 -> NO ANY CONFIG LINES BEFORE! **
#define CVER_VAL    (*(unsigned *)(config_list[0].val))     /* L/R value of the version */
 { _T("VER_CONFIG"),    VCFG_UNSIGNED,  &the.cfg.ver_config,                        &DB_ver_config },

// BOOL is_wav_support; true, if the module support WAV
 { _T("WAV_SUPPORT"),   VCFG_BOOL,      &the.cfg.is_wav_support,                    &DB_is_wav_support },

// BOOL is_rwave_support; true, if the module support RWAVE ext. for WAV
 { _T("RWAVE_SUPPORT"), VCFG_BOOL,      &the.cfg.is_rwave_support,                  &DB_is_rwave_support },

// unsigned infobox_parenting; value INFOBOX_xxx - behaviour of "Alt-3" window
 { _T("IBOX_PARENT"),   VCFG_UNSIGNED,  &the.cfg.infobox_parenting,                 &DB_infobox_parenting },

// BOOL enable_unload_cleanup; enable cleanup on unload plugin
 { _T("LAST_CHANCE"),   VCFG_BOOL,      &the.cfg.enable_unload_cleanup,             &DB_enable_unload_cleanup },

// unsigned play_sleep; sleep while playback, ms
 { _T("PLAY_SLEEP"),    VCFG_UNSIGNED,  &the.cfg.play_sleep,                        &DB_play_sleep },

// BOOL disable_play_sleep; disable sleep on plyback
 { _T("DISABLE_SLEEP"), VCFG_BOOL,      &the.cfg.disable_play_sleep,                &DB_disable_play_sleep },

// unsigned sec_align; time in seconds to align file length
 { _T("SEC_ALIGN"),     VCFG_UNSIGNED,  &the.cfg.sec_align,                         &DB_sec_align },

// unsigned fade_in; track fade in, ms
 { _T("FADE_IN"),       VCFG_UNSIGNED,  &the.cfg.fade_in,                           &DB_fade_in },

// unsigned fade_in; track fade in, ms
 { _T("FADE_OUT"),      VCFG_UNSIGNED,  &the.cfg.fade_out,                          &DB_fade_out },

// BOOL is_frmod_scaled; true, if unsigned scaled modulation frequencies in use
 { _T("FRMOD_SCALED"),  VCFG_BOOL,      &the.cfg.is_frmod_scaled,                   &DB_is_frmod_scaled },

// unsigned iir_filter_no; number of current HB LPF for quad
 { _T("IIR_HBLPF_IX"),  VCFG_UNSIGNED,  &the.cfg.iir_filter_no,                     &DB_iir_filter_no },

// BOOL iir_comp_config.is_kahan; type of process IIR computation (Kahan / baseline summation)
 { _T("IIR_SUM_KAHAN"), VCFG_BOOL,      &the.cfg.iir_comp_config.is_kahan,          &DB_iir_sum_kahan },

// BOOL iir_comp_config.is_subnorm_reject; suppressing low values in "delay line" of IIR state
 { _T("IIR_SUBN_ZERO"), VCFG_BOOL,      &the.cfg.iir_comp_config.is_subnorm_reject, &DB_iir_subnorm_reject },

// double iir_comp_config.subnorm_thr; subnorm reject threshold
 { _T("IIR_SUBN_THR"),  VCFG_DOUBLE_BIN, &the.cfg.iir_comp_config.subnorm_thr,      &DB_iir_subnorm_thr },

// BOOL is_clr_nframe_trk; TRUE when we want to clear frame counter per each track
 { _T("CLR_NFRAME_PT"), VCFG_BOOL,      &the.cfg.is_clr_nframe_trk,                 &DB_is_clr_nframe_trk },

// BOOL is_clr_hilb_trk; TRUE when we want to clear analitic transformers per each track
 { _T("CLR_HILB_PT"),   VCFG_BOOL,      &the.cfg.is_clr_hilb_trk,                   &DB_is_clr_hilb_trk },

// BOOL show_long_numbers; FALSE -- short format, TRUE -- %G or something same
 { _T("SHOW_LONGNUMB"), VCFG_BOOL,      &the.cfg.show_long_numbers,                 &DB_show_long_numbers },

// BOOL is_fp_check; true, if floating point check set
 { _T("FP_CHECK"),      VCFG_BOOL,      &the.cfg.is_fp_check,                       &DB_is_fp_check },

// BOOL is24bits; FALSE -- 16 bits decoding (real output of playback/transcode)
 { _T("NEED24BITS"),    VCFG_BOOL,      &the.cfg.need24bits,                        &DB_need24bits },

// double sr_config.dth_bits; bits to dither (+-LSbits; _TPDF reduced_)
 { _T("DITHER_BITS"),   VCFG_DOUBLE_BIN, &the.cfg.sr_config.dth_bits,               &DB_dth_bits },

// unsigned quantz_type; type of rounding while convert double to int
 { _T("QUANTIZE_TYPE"), VCFG_UNSIGNED,  &the.cfg.sr_config.quantz_type,             &DB_quantz_type },

// unsigned sr_config.render_type; type of dithering (rendering) while convert double to int
 { _T("RENDER_TYPE"),   VCFG_UNSIGNED,  &the.cfg.sr_config.render_type,             &DB_render_type },

// unsigned sr_config.nshape_type; noise shaping type SND_NSHAPE_xxx
 { _T("NOISE_SHAPING"), VCFG_UNSIGNED,  &the.cfg.sr_config.nshape_type,             &DB_nshape_type },

// unsigned sr_config.sign_bits16; significant bits for 16bit output
 { _T("SIGNBITS16"),    VCFG_UNSIGNED,  &the.cfg.sr_config.sign_bits16,             &DB_sign_bits16},

// unsigned sr_config.sign_bits24; significant bits for 24bit output
 { _T("SIGNBITS24"),    VCFG_UNSIGNED,  &the.cfg.sr_config.sign_bits24,             &DB_sign_bits24},

// NODE_DSP *dsp_list; the copy of last dsp-list
 { _T("NODE_DSP"),      VCFG_FUNCTION,  &the.cfg.dsp_list,                          &handle_node_dsp },

// the end of list
 { NULL,                0,              NULL,                                       NULL }
};

/* read and pre-parse config line "keyword=tail", blanks around _not ignored_, but
 * empty strings skipped and string with control chars other then '\n' '\r' '\t'
 * treated as error
 */
static BOOL read_conf_line(FILE *fp, TCHAR *buf, TCHAR **keyword, TCHAR **tail)
{
 size_t cnt;
 TCHAR *p;
 _TINT c;
 BOOL blanks_only;

 *keyword = *tail = NULL;                       // EOF state

 for(;;)                                        // loop by (empty) lines
 {
  cnt = 0;
  p = buf;
  *keyword = *tail = NULL;

  while((c = _gettc(fp)) != _TEOF)
  {
   if(_T('\r') == c)                            // simple ignore by a weak way
    continue;
   if(_T('\n') == c)
    break;

  // don't keep tabs -- isblank() live at high-level routines
  *p++ = (c == _T('\t'))? _T(' ') : c;
  if(++cnt >= MAX_CONFIG_LINE)
   return FALSE;
  }
  *p = _T('\0');

  blanks_only = TRUE;
  for(p = buf; *p; ++p)
  {
   if(_istcntrl(*p))
    return FALSE;
   if(_istgraph(*p))
    blanks_only = FALSE;
  }

  if(blanks_only)
  {
   if(c == _TEOF)
    return TRUE;
  }
  else
  {
   // non-blank non-control chars string
   if(NULL != (*tail = _tcschr(buf, _T('='))))
   {
    *((*tail)++) = _T('\0');
    *keyword = buf;
    return TRUE;
   }
   else
   {
    return FALSE;
   }
  }
 }
}

/* write config line "keyword=tail". Assumed that *tail begin from ' '
*/
static BOOL write_conf_line(FILE *fp, const CONFIG *cfg_node, const TCHAR *tail)
{
 // tail+1 because the 1st symbol in *tail is always ' '
 return (_ftprintf(fp, _T("%s=%s\n"), cfg_node -> name, tail + 1) > 0
        && !ferror(fp));
}

/* handle string value (on *READ* initially pval can be equv. buf, i.e. pval = *tbuf)
*/
static void handle_string(TCHAR **buf, TCHAR *pval, size_t max_size, BOOL toWrite)
{
// The string begin from any non-blank char. The string terminated  by blank char or '\0'.
// If the string contain blank(s), each escaped by the '%': "abc% def" or "% % zxy% ".
// The '%' escaped by '%'; if after '%' going non-blank and non-%, '%' ignored.
// "%%% +%ab%" == "% +ab"
 size_t cnt;

 if(toWrite)
 {
  *(*buf)++ = _T(' ');
  cnt = 1;
  while(*pval && cnt < max_size - 2)                    // keep a space for ESC('%')
  {
   if(_istblank(*pval) || _T('%') == *pval)
   {
    *(*buf)++ = _T('%');
    *(*buf)++ = *pval++;
    cnt += 2;
   }
   else
   {
    *(*buf)++ = *pval++;
    cnt += 1;
   }
  }
  **buf = _T('\0');
 }
 else
 {
  cnt = 0;
  while(**buf && _istblank(**buf))
   ++(*buf);

  while(**buf && !_istblank(**buf))
  {
   if(_T('%') == **buf)
   {
    ++(*buf);
    if(_istblank(**buf) || _T('%') == **buf)
    {
     if(cnt < max_size - 2)
     {
      *pval++ = *(*buf)++;
      ++cnt;
     }
     else
      ++(*buf);
    }
   }
   else
   {
    if(cnt < max_size - 2)
    {
     *pval++ = *(*buf)++;
     ++cnt;
    }
    else
     ++(*buf);
   }
  }

  if(**buf)                                     // *buf == ptr to the rest of string
   ++(*buf);

  *pval = _T('\0');
 }
}

/* handle BOOL value
*/
static BOOL handle_bool(TCHAR **buf, volatile BOOL *pval, BOOL toWrite)
{
 if(toWrite)
 {
  *buf += _stprintf(*buf, _T(" %d"), *pval? 1 : 0);
  return TRUE;
 }
 else
 {
  TCHAR *val_buf = *buf;
  int val;

  handle_string(buf, val_buf, MAX_CONFIG_ARGS, toWrite);
  return (_stscanf(val_buf, _T("%d"), &val) != 1)?
        FALSE
        :
        ((*pval = (val? TRUE : FALSE)), TRUE);
 }
}

/* handle int value
*/
BOOL handle_int(TCHAR **buf, volatile int *pval, BOOL toWrite)
{
 if(toWrite)
 {
  *buf += _stprintf(*buf, _T(" %d"), *pval);
  return TRUE;
 }
 else
 {
  TCHAR *val_buf = *buf;
  int val;

  handle_string(buf, val_buf, MAX_CONFIG_ARGS, toWrite);
  return (_stscanf(val_buf, _T("%d"), &val) != 1)?
        FALSE
        :
        ((*pval = val), TRUE);
 }
}

/* handle unsigned value
*/
BOOL handle_unsigned(TCHAR **buf, volatile unsigned *pval, BOOL toWrite)
{
 if(toWrite)
 {
  *buf += _stprintf(*buf, _T(" %u"), *pval);
  return TRUE;
 }
 else
 {
  TCHAR *val_buf = *buf;
  unsigned val;

  handle_string(buf, val_buf, MAX_CONFIG_ARGS, toWrite);
  return (_stscanf(val_buf, _T("%u"), &val) != 1)?
        FALSE
        :
        ((*pval = val), TRUE);
 }
}

/* handle double-as-double value
*/
BOOL handle_double(TCHAR **buf, volatile double *pval, BOOL toWrite)
{
 if(toWrite)
 {
  *buf += _stprintf(*buf, _T(" %.18G"), *pval);
  return TRUE;
 }
 else
 {
  TCHAR *val_buf = *buf;
  double val;

  handle_string(buf, val_buf, MAX_CONFIG_ARGS, toWrite);
  if(_T('0') == val_buf[0] && (_T('x') == val_buf[1] || _T('X') == val_buf[1]))
  {
   return (_stscanf(val_buf + 2, _T("%I64x"), (uint64_t *)&val) != 1)?
        FALSE
        :
        ((*pval = val), TRUE);
  }
  else
  {
   return (_stscanf(val_buf, _T("%Lg"), &val) != 1)?
        FALSE
        :
        ((*pval = val), TRUE);
  }
 }
}

/* handle double-as-64-bits-binary value
*/
BOOL handle_double_bin(TCHAR **buf, volatile double *pval, BOOL toWrite)
{
 if(toWrite)
 {
  *buf += _stprintf(*buf, _T(" 0x%08I64X"), *(uint64_t *)pval);
  return TRUE;
 }
 else
 {
  return handle_double(buf, pval, toWrite);
 }
}

/* handle config's NIDE_DSP type
*/
static BOOL handle_node_dsp(FILE *fp, TCHAR **buf,
        const CONFIG *cfg_node, BOOL toWrite, BOOL is_empty)
{
// .name, .lgain, .rgain, .lock_gain, non-zero .inputs as BOOL[N_INPUTS], .mode, .dsp.*
 NODE_DSP **ppn = (NODE_DSP **)(cfg_node -> val);
 NODE_DSP *cur;
 BOOL input;
 int i;

 if(toWrite)
 {
  BOOL is_ok = TRUE;

  if(!buf)                                      // can't be here
   return FALSE;

  cur = *ppn;                                   // dsp list head
  while(cur)                                    // can't be empty
  {
   if(is_ok)
   {
    TCHAR *args = *buf;

    // the summary length of config string _must_ be shoter than MAX_CONFIG_ARGS
    handle_string    (&args, cur -> name, SIZE_DSP_NAME, toWrite);

    handle_double_bin(&args, &(cur -> l_gain   ), toWrite);
    handle_double_bin(&args, &(cur -> r_gain   ), toWrite);
    handle_bool      (&args, &(cur -> lock_gain), toWrite);

    for(i = 0; i < N_INPUTS; ++i)
    {
     input = cur -> inputs[i];
     handle_bool(&args, &input, toWrite);
    }

    handle_int(&args,  &(cur -> xch_mode   ), toWrite);
    handle_bool(&args, &(cur -> l_iq_invert), toWrite);
    handle_bool(&args, &(cur -> r_iq_invert), toWrite);

    handle_int(&args, &(cur -> mode), toWrite);
    switch(cur -> mode)
    {
     case MODE_MASTER:
      handle_int(&args, &(cur -> dsp.mk_master.le.tout), toWrite);
      handle_int(&args, &(cur -> dsp.mk_master.ri.tout), toWrite);
      break;

     case MODE_SHIFT:
      handle_double_bin(&args, &(cur -> dsp.mk_shift.le.fr_shift), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_shift.le.is_shift), toWrite);

      handle_double_bin(&args, &(cur -> dsp.mk_shift.ri.fr_shift), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_shift.ri.is_shift), toWrite);

      handle_int       (&args, &(cur -> dsp.mk_shift.n_out          ), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_shift.lock_shift     ), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_shift.sign_lock_shift), toWrite);
      break;

     case MODE_PM:
      handle_double_bin(&args, &(cur -> dsp.mk_pm.le.freq ), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.le.phase), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.le.level), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.le.angle), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.le.is_pm), toWrite);

      handle_double_bin(&args, &(cur -> dsp.mk_pm.ri.freq ), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.ri.phase), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.ri.level), toWrite);
      handle_double_bin(&args, &(cur -> dsp.mk_pm.ri.angle), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.ri.is_pm), toWrite);

      handle_int       (&args, &(cur -> dsp.mk_pm.n_out     ), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.lock_freq ), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.lock_phase), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.lock_level), toWrite);
      handle_bool      (&args, &(cur -> dsp.mk_pm.lock_angle), toWrite);
      break;

     case MODE_MIX:
      handle_int(&args, &(cur -> dsp.mk_mix.n_out), toWrite);
      break;
    }

    is_ok = write_conf_line(fp, cfg_node, *buf);
   }

   cur = cur -> next;                           // free the node
   free(*ppn);
   *ppn = cur;                                  // will be NULL at the end
  }
  return is_ok;
 }
 else
 {
  NODE_DSP temp;

  if(!buf)
  {
   if(is_empty)
   {
    // simple clear uninitialied NODE_DSP list
    *ppn = NULL;
   }
   else
   {
    // free NODE_DSP list and clear it's config pointer
    while(*ppn)
    {
     cur = (*ppn) -> next;
     free(*ppn);
     *ppn = cur;                                // will be NULL at the end
    }
   }
   return TRUE;
  }

// short and easy road to hell
#define HANDLE_VAL(T, V) if(!handle_##T(buf, &(V), toWrite)) return FALSE
#define HANDLE_CHK(T, V, MI, MA) HANDLE_VAL(T, V); if((V) < (MI)) (V) = (MI); if((V) > (MA)) (V) = (MA)

  memset(&temp, 0, sizeof(temp));

  // difference from the other handle_XXX routines: we have to write to parameter variable before the checks
  // will complete; so if we can return FALSE, the content of *pval is undefined
  handle_string(buf, temp.name, SIZE_DSP_NAME, toWrite);

  HANDLE_CHK(double, temp.l_gain, 0.0, MAX_GAIN);
  HANDLE_CHK(double, temp.r_gain, 0.0, MAX_GAIN);
  HANDLE_VAL(bool,   temp.lock_gain);

  for(i = 0; i < N_INPUTS; ++i)
  {
   HANDLE_VAL(bool, input);
   temp.inputs[i] = (char)input;
  }

  HANDLE_CHK(int,  temp.xch_mode, XCH_NORMAL, XCH_MAX);
  HANDLE_VAL(bool, temp.l_iq_invert);
  HANDLE_VAL(bool, temp.r_iq_invert);

  HANDLE_CHK(int, temp.mode, MODE_MASTER, MODE_MIX);
  switch(temp.mode)
  {
   case MODE_MASTER:
    HANDLE_CHK(int, temp.dsp.mk_master.le.tout, S_RE, S_SUM_REIM);
    HANDLE_CHK(int, temp.dsp.mk_master.ri.tout, S_RE, S_SUM_REIM);
    break;

   case MODE_SHIFT:
    HANDLE_CHK(double, temp.dsp.mk_shift.le.fr_shift, -MAX_FSHIFT, MAX_FSHIFT);
    HANDLE_VAL(bool,   temp.dsp.mk_shift.le.is_shift);

    HANDLE_CHK(double, temp.dsp.mk_shift.ri.fr_shift, -MAX_FSHIFT, MAX_FSHIFT);
    HANDLE_VAL(bool,   temp.dsp.mk_shift.ri.is_shift);

    HANDLE_CHK(int,    temp.dsp.mk_shift.n_out, 1, N_INPUTS);
    HANDLE_VAL(bool,   temp.dsp.mk_shift.lock_shift);
    HANDLE_VAL(bool,   temp.dsp.mk_shift.sign_lock_shift);
    break;

   case MODE_PM:
    HANDLE_CHK(double, temp.dsp.mk_pm.le.freq,  0.0,         MAX_PMFREQ);
    HANDLE_CHK(double, temp.dsp.mk_pm.le.phase, MIN_PMPHASE, MAX_PMPHASE);
    HANDLE_CHK(double, temp.dsp.mk_pm.le.level, 0.0,         MAX_PMLEVEL);
    HANDLE_CHK(double, temp.dsp.mk_pm.le.angle, MIN_PMANGLE, MAX_PMANGLE);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.le.is_pm);

    HANDLE_CHK(double, temp.dsp.mk_pm.ri.freq,  0.0,         MAX_PMFREQ);
    HANDLE_CHK(double, temp.dsp.mk_pm.ri.phase, MIN_PMPHASE, MAX_PMPHASE);
    HANDLE_CHK(double, temp.dsp.mk_pm.ri.level, 0.0,         MAX_PMLEVEL);
    HANDLE_CHK(double, temp.dsp.mk_pm.ri.angle, MIN_PMANGLE, MAX_PMANGLE);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.ri.is_pm);

    HANDLE_CHK(int,    temp.dsp.mk_pm.n_out, 1, N_INPUTS);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.lock_freq);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.lock_phase);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.lock_level);
    HANDLE_VAL(bool,   temp.dsp.mk_pm.lock_angle);
    break;

   case MODE_MIX:
    HANDLE_CHK(int, temp.dsp.mk_mix.n_out, 1, N_INPUTS);
    break;

   default:
    return FALSE;
  }

  // we have _completely_ filled NODE_DSP temp (w/o links); so we are ready to include it to the list
  if(*ppn)
  {
   cur = *ppn;
   while(cur -> next)
    cur = cur -> next;
   cur -> next = cmalloc(sizeof(NODE_DSP));
   memcpy(cur -> next, &temp, sizeof(NODE_DSP));
   cur -> next -> next = NULL;
   cur -> next -> prev = cur;
  }
  else
  {
   *ppn = cmalloc(sizeof(NODE_DSP));
   memcpy(*ppn, &temp, sizeof(NODE_DSP));
   (*ppn) -> prev = (*ppn) -> next = NULL;
  }

  return TRUE;
 }
#undef HANDLE_CHK
#undef HANDLE_VAL
}


/* reset the config to default (for internal config needs only!)
*/
void reset_config(BOOL is_empty)
{
 const CONFIG *cfg_node;

 for(cfg_node = config_list; cfg_node -> name; ++cfg_node)
 {
  switch(cfg_node -> type)
  {
   case VCFG_BOOL:
    *(BOOL *)(cfg_node -> val) = *(BOOL *)(cfg_node -> db_or_fun);
    break;
   case VCFG_INT:
    *(int *)(cfg_node -> val) = ((DB_VAUES_INT *)(cfg_node -> db_or_fun)) -> v_def;
    break;
   case VCFG_UNSIGNED:
    *(unsigned *)(cfg_node -> val) = ((DB_VAUES_UNSIGNED *)(cfg_node -> db_or_fun)) -> v_def;
    break;
   case VCFG_DOUBLE:
   case VCFG_DOUBLE_BIN:
    *(double *)(cfg_node -> val) = ((DB_VAUES_DOUBLE *)(cfg_node -> db_or_fun)) -> v_def;
    break;
   case VCFG_STRING:
    *(const TCHAR **)(cfg_node -> val) = *(const TCHAR **)(cfg_node -> db_or_fun);
    break;
   case VCFG_FUNCTION:
    (*(SPEC_CFG_RW)(cfg_node -> db_or_fun))(NULL, NULL, cfg_node, FALSE, is_empty);
    break;
   default:                             // processing don't need
    break;
  }
 }
}

/* The front-end interface
 * --- --------- ---------
 */
/* load the config by file name
*/
BOOL load_config(const TCHAR *config_name)
{
 const CONFIG *cfg_node;
 FILE *fp = NULL;
 TCHAR *conf_line = NULL, *keyword = NULL, *tail = NULL;
 BOOL is_ok;

 // set defaults
 reset_config(TRUE);                                        // .version incorrect!!

 // open a file and prepare to per-line reading
 if(NULL == (fp = _tfopen(config_name, _T("r") FOPEN_FLG)))
 {
  CVER_VAL = VERSION_CONFIG;                                // somewhat oddily
  return FALSE;
 }

 conf_line = cmalloc((MAX_CONFIG_LINE + 1) * sizeof(TCHAR));

 // silly and stupid per-line reading (where is a map<>?) -- up to the *FIRST* error
 for(;;)
 {
  TCHAR *tptr;
  BOOL is_found;

// yet another road to hell
#define NODE_CHK(T, TT) if(is_ok)                                                       \
        {                                                                               \
         if(*(T *)(cfg_node -> val) < ((TT *)(cfg_node -> db_or_fun)) -> v_min)         \
          *(T *)(cfg_node -> val) = ((TT *)(cfg_node -> db_or_fun)) -> v_min;           \
         if(*(T *)(cfg_node -> val) > ((TT *)(cfg_node -> db_or_fun)) -> v_max)         \
          *(T *)(cfg_node -> val) = ((TT *)(cfg_node -> db_or_fun)) -> v_max;           \
        }

  is_ok = read_conf_line(fp, conf_line, &keyword, &tail);
  if(!is_ok || !keyword || !tail)
   break;
  tptr = keyword;
  handle_string(&tptr, keyword, MAX_CONFIG_KEYW, FALSE);

  for(is_found = FALSE, cfg_node = config_list; !is_found && cfg_node -> name; ++cfg_node)
  {
   if(!_tcsicmp(keyword, cfg_node -> name))
   {
    is_found = TRUE;
    switch(cfg_node -> type)
    {
     case VCFG_BOOL:
      is_ok = handle_bool(&tail, cfg_node -> val, FALSE);
      break;
     case VCFG_INT:
      is_ok = handle_int(&tail, cfg_node -> val, FALSE);
      NODE_CHK(int, DB_VAUES_INT)
      break;
     case VCFG_UNSIGNED:
      is_ok = handle_unsigned(&tail, cfg_node -> val, FALSE);
      NODE_CHK(unsigned, DB_VAUES_UNSIGNED)
      break;
     case VCFG_DOUBLE:
     case VCFG_DOUBLE_BIN:
      is_ok = handle_double(&tail, cfg_node -> val, FALSE);
      NODE_CHK(double, DB_VAUES_DOUBLE)
      break;
     case VCFG_STRING:
      // may be "". may be up to MAX_CONFIG_ARGS length. The best choise - to avoid this
      handle_string(&tail, *(TCHAR **)(cfg_node -> val), MAX_CONFIG_ARGS, FALSE);
      break;
     case VCFG_FUNCTION:
      is_ok = (*(SPEC_CFG_RW)(cfg_node -> db_or_fun))(fp, &tail, cfg_node, FALSE, FALSE);
      break;
     default:                           // processing don't need
      is_ok = FALSE;
      break;
    }
    break;
   }
  }
  if(!is_found)
  {
   is_ok = FALSE;
   break;
  }
 }

 free(conf_line);
 fclose(fp);

 // config version check
 if((CVER_VAL != VERSION_CONFIG) || !is_ok)
 {
  reset_config(FALSE);
  is_ok = FALSE;
  CVER_VAL = VERSION_CONFIG;
 }

 return is_ok;

#undef NODE_CHK
}

/* save the config by the file name
*/
BOOL save_config(const TCHAR *config_name)
{
 const CONFIG *cfg_node = config_list;
 FILE *fp = NULL;
 TCHAR *conf_line = NULL;
 BOOL ret_val = FALSE;

 if(NULL == (fp = _tfopen(config_name, _T("w") FOPEN_FLG)))
  return FALSE;
 conf_line = cmalloc((MAX_CONFIG_LINE + 1) * sizeof(TCHAR));

 while(cfg_node -> name)
 {
  TCHAR *tail = conf_line;

  switch(cfg_node -> type)
  {
   case VCFG_BOOL:
    ret_val = handle_bool(&tail, (BOOL *)(cfg_node -> val), TRUE);
    break;
   case VCFG_INT:
    ret_val = handle_int(&tail, (int *)(cfg_node -> val), TRUE);
    break;
   case VCFG_UNSIGNED:
    ret_val = handle_unsigned(&tail, (unsigned *)(cfg_node -> val), TRUE);
    break;
   case VCFG_DOUBLE:
    ret_val = handle_double(&tail, (double *)(cfg_node -> val), TRUE);
    break;
   case VCFG_DOUBLE_BIN:
    ret_val = handle_double_bin(&tail, (double *)(cfg_node -> val), TRUE);
    break;
   case VCFG_STRING:
    handle_string(&tail, (TCHAR *)(cfg_node -> val), MAX_CONFIG_ARGS - 2, TRUE);
    ret_val = TRUE;
    break;
   case VCFG_FUNCTION:
    ret_val = (*(SPEC_CFG_RW)(cfg_node -> db_or_fun))(fp, &tail, cfg_node, TRUE, FALSE);
    break;
   default:                                     // processing don't need
    break;
  }

  if(ret_val)
  {
   if(cfg_node -> type != VCFG_FUNCTION)        // real write to config file
    ret_val = write_conf_line(fp, cfg_node, conf_line);

   ++cfg_node;
  }
  if(!ret_val)
   break;
 }

 free(conf_line);
 fclose(fp);
 return ret_val;
}

/* helper -- get default config file name (need free())
*/
static TCHAR *get_def_config_name(void)
{
 TCHAR *config_name = cmalloc((MAX_FILE_PATH + 1) * sizeof(TCHAR));
 BOOL dir_res;
 unsigned name_len;
 // can't make global == unsure about calling quit()
 HRESULT com_init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
 HRESULT res_folder = SHGetFolderPath(NULL,
        CSIDL_APPDATA | CSIDL_FLAG_CREATE,
        NULL,
        SHGFP_TYPE_CURRENT,
        config_name);

 if(SUCCEEDED(com_init))
  CoUninitialize();

 if(FAILED(res_folder))
 {
  free(config_name);
  config_name = NULL;
  return NULL;
 }

 name_len = _tcslen(config_name);
 if(name_len)
 {
  if(config_name[name_len - 1] == _T('\\'))
   config_name[name_len - 1] = _T('\0');
 }
 _tcscat(config_name, _T("\\in_cwave"));

 dir_res = CreateDirectory(config_name, NULL);
 if(!dir_res && GetLastError() != ERROR_ALREADY_EXISTS)
 {
  free(config_name);
  config_name = NULL;
 }
 else
  _tcscat(config_name, _T("\\in_cwave.cfg"));

 return config_name;
}

/* load default config
*/
BOOL load_config_default(void)
{
 the.def_conf_filename = get_def_config_name();

 if(the.def_conf_filename)
 {
  BOOL res = load_config(the.def_conf_filename);
  return res;
 }
 return FALSE;
}

/* save default config
*/
BOOL save_config_default(void)
{
 // don't call get_def_config_name() -- it uses MS COM stuff but this can be called from DllMain()
 if(the.def_conf_filename)
 {
  BOOL res = save_config(the.def_conf_filename);
  free(the.def_conf_filename);
  the.def_conf_filename = NULL;
  return res;
 }
 return FALSE;
}


/* The End...
*/

