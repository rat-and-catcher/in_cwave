/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      in_cwave.c -- the DllMain, module-wide 'the' stuff
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

/* Global variables
 * ------ ---------
 */
// The Our All
IN_CWAVE the;


/* The modulator initialization
 * --- --------- --------------
 */
/* init one copy of MOD_CONTEXT (modulator context, not a "module")
*/
static void mod_context_init(MOD_CONTEXT *mc)
{
 int i;

 // frames (samples) counter
 InitializeCriticalSection(&(mc -> cs_n_frame));
 mod_context_reset_framecnt(mc);

 // in/out buses
 for(i = 0; i < N_INPUTS; ++i)
 {
  mc -> inout[i].le.re  = mc -> inout[i].le.im =
  mc -> inout[i].ri.re  = mc -> inout[i].ri.im = 0.0;
 }

 // analitic signal convertors
 mc -> h_left  = hq_rp_create_ix(the.cfg.iir_filter_no, the.cfg.iir_sum_kahan);
 mc -> h_right = hq_rp_create_ix(the.cfg.iir_filter_no, the.cfg.iir_sum_kahan);

 mc -> xr = NULL;                               // now we don't have a file

 // sound renders -- two and only two / here and only here
 // seeds set here and only here -- like a chance to get exactly predicable results
 sound_render_init(&the.cfg.sr_config, the.cfg.need24bits, 0x13579BDF, &mc -> sr_left);
 sound_render_init(&the.cfg.sr_config, the.cfg.need24bits, 0x479B22AB, &mc -> sr_right);

 mc ->      fes_hilb_left .is_enable
    = mc -> fes_hilb_right.is_enable
    = mc -> fes_sr_left   .is_enable
    = mc -> fes_sr_right  .is_enable = !!(the.cfg.is_fp_check);
 except_stats_reset(&(mc -> fes_hilb_left));
 except_stats_reset(&(mc -> fes_hilb_right));
 except_stats_reset(&(mc -> fes_sr_left));
 except_stats_reset(&(mc -> fes_sr_right));
}

/* cleanup one copy of MOD_CONTEXT (modulator context, not a "module")
*/
static void mod_context_cleanup(MOD_CONTEXT *mc)
{
 // there is no other threads in this time (hopelly)

 // non - permanent objects
 mod_context_fclose(mc);

 // permanent objects
 if(mc -> h_left)
 {
  hq_rp_destroy(mc -> h_left);
  mc -> h_left = NULL;
 }
 if(mc -> h_right)
 {
  hq_rp_destroy(mc -> h_right);
  mc -> h_right = NULL;
 }

 DeleteCriticalSection(&(mc -> cs_n_frame));

 sound_render_cleanup(&(mc -> sr_left));
 sound_render_cleanup(&(mc -> sr_right));
}

/* reset modulator's Hilbert transformer
*/
static void mod_context_hilbert_reset(MOD_CONTEXT *mc)
{
 hq_rp_reset(mc -> h_left);
 hq_rp_reset(mc -> h_right);
}

/* change summation algorithm of MOD_CONTEXT according 'the'
*/
static void mod_context_hilnert_iir_sum_chanage(MOD_CONTEXT *mc)
{
 hq_rp_setsum(mc -> h_left,  the.cfg.iir_sum_kahan);
 hq_rp_setsum(mc -> h_right, the.cfg.iir_sum_kahan);
}

/* change Hilbert taransformer parameters in MOD_CONTEXT according 'the'
*/
static void mod_context_hilbert_change(MOD_CONTEXT *mc)
{
 if(mc -> h_left)
 {
  hq_rp_destroy(mc -> h_left);
  mc -> h_left = NULL;
 }
 if(mc -> h_right)
 {
  hq_rp_destroy(mc -> h_right);
  mc -> h_right = NULL;
 }
 mc -> h_left  = hq_rp_create_ix(the.cfg.iir_filter_no, the.cfg.iir_sum_kahan);
 mc -> h_right = hq_rp_create_ix(the.cfg.iir_filter_no, the.cfg.iir_sum_kahan);
}

/* Work with Hilbert's transformers
 * ---- ---- --------- ------------
 */
/* lock Hilberts transformers from change
*/
void all_hilberts_lock(void)
{
 EnterCriticalSection(&the.cs_hilbert_change);
}

/* unlock Hilberts transformers to change
*/
void all_hilberts_unlock(void)
{
 LeaveCriticalSection(&the.cs_hilbert_change);
}

/* reset the modulator-context's Hilbert's transformer
*/
void mod_context_reset_hilbert(MOD_CONTEXT *mc)
{
 all_hilberts_lock();
 mod_context_hilbert_reset(mc);
 all_hilberts_unlock();
}

/* change summation algorithm in IIR-filters in Hilbert transformer
*/
void mod_context_change_all_hilberts_summation(BOOL new_sum_kahan)
{
 if(the.cfg.iir_sum_kahan != new_sum_kahan)
 {
  all_hilberts_lock();

  the.cfg.iir_sum_kahan = new_sum_kahan;
  mod_context_hilnert_iir_sum_chanage(&the.mc_playback);
  mod_context_hilnert_iir_sum_chanage(&the.mc_transcode);

  all_hilberts_unlock();
 }
}

/* change the current Hilbert's transformers
*/
void mod_context_change_all_hilberts_filter(unsigned new_filter_no)
{
 if(the.cfg.iir_filter_no != new_filter_no
        && new_filter_no <= IX_IIR_LOEL_TYPEMAX)
 {
  all_hilberts_lock();

  the.cfg.iir_filter_no = new_filter_no;
  mod_context_hilbert_change(&the.mc_playback);
  mod_context_hilbert_change(&the.mc_transcode);

  all_hilberts_unlock();
 }
}


/* The MOD_CONTEXT as a file
 * --- ----------- -- - ----
 */
/* open a file to processing with a MOD_CONTEXT object
*/
BOOL mod_context_fopen(const TCHAR *name, unsigned read_quant,  // for XWAVE_READER
        MOD_CONTEXT *mc)
{
 // ** don't need any locks before return from here **

 // fix the render params for the current transcode
 BOOL need24bits = the.cfg.need24bits;

 // the reader first
 if(NULL == (mc -> xr = xwave_reader_create(
      name
    , read_quant
    , the.cfg.sec_align
    , the.cfg.fade_in
    , the.cfg.fade_out
    )))
  return FALSE;

 // clear frame counter / analitic transformers if need
 if(the.cfg.is_clr_nframe_trk)
  mod_context_reset_framecnt(mc);
 if(the.cfg.is_clr_hilb_trk)
  mod_context_reset_hilbert(mc);

 // create the renders -- no playback -- we can/must to change bits according to
 // current config -- here and only here (other parameters can be changed in "realtime")
 sound_render_set_outbits(need24bits, &mc -> sr_left);
 sound_render_set_outbits(need24bits, &mc -> sr_right);
 return TRUE;
}

/* close a file was processing with a MOD_CONTEXT object
*/
void mod_context_fclose(MOD_CONTEXT *mc)
{
 // simple close input file (if any)
 if(mc -> xr)
 {
  xwave_reader_destroy(mc -> xr);
  mc -> xr = NULL;
 }
}

/* MOD_CONTEXT -- miscellaneous
 * ----------- -- -------------
 */
/* clear (unused) in-out in the all module-contexts
*/
void mod_context_clear_all_inouts(int nclr)
{
  the.mc_playback.inout [nclr].le.re = the.mc_playback.inout [nclr].le.im =
  the.mc_playback.inout [nclr].ri.re = the.mc_playback.inout [nclr].ri.im =
  the.mc_transcode.inout[nclr].le.re = the.mc_transcode.inout[nclr].le.im =
  the.mc_transcode.inout[nclr].ri.re = the.mc_transcode.inout[nclr].ri.im = 0.0;
}

/* lock frame counter
*/
void mod_context_lock_framecnt(MOD_CONTEXT *mc)
{
 EnterCriticalSection(&(mc -> cs_n_frame));
}

/* unlock frame counter
*/
void mod_context_unlock_framecnt(MOD_CONTEXT *mc)
{
 LeaveCriticalSection(&(mc -> cs_n_frame));
}

/* get value of frame counter
*/
uint64_t mod_context_get_framecnt(MOD_CONTEXT *mc)
{
 uint64_t res;

 mod_context_lock_framecnt(mc);
 res = mc -> n_frame;
 mod_context_unlock_framecnt(mc);
 return res;
}

/* clear frame counter
*/
void mod_context_reset_framecnt(MOD_CONTEXT *mc)
{
 mod_context_lock_framecnt(mc);
 mc -> n_frame = 0ULL;
 mod_context_unlock_framecnt(mc);
}

/* modulators exceptions statistics -- summary
 * ---------- ---------- ---------- -- -------
 */
/* set new mode for the all FP exception counters
*/
void fecs_set_endis_all(BOOL new_endis)
{
// do not need any locks
 if((!!new_endis) != (!!the.cfg.is_fp_check))
 {
    the.mc_playback.fes_hilb_left.is_enable
  = the.mc_playback.fes_hilb_right.is_enable
  = the.mc_playback.fes_sr_left.is_enable
  = the.mc_playback.fes_sr_right.is_enable

  = the.mc_transcode.fes_hilb_left.is_enable
  = the.mc_transcode.fes_hilb_right.is_enable
  = the.mc_transcode.fes_sr_left.is_enable
  = the.mc_transcode.fes_sr_right.is_enable

  = the.cfg.is_fp_check

  = new_endis;

  fecs_resets(TRUE, TRUE, TRUE, TRUE);
 }
}

/* reset float exceptions counters by group
*/
void fecs_resets(BOOL res_hilb_le, BOOL res_hilb_ri, BOOL res_sr_le, BOOL res_sr_ri)
{
// do not need any locks
 if(res_hilb_le)
 {
  except_stats_reset(&(the.mc_playback .fes_hilb_left));
  except_stats_reset(&(the.mc_transcode.fes_hilb_left));
 }

 if(res_hilb_ri)
 {
  except_stats_reset(&(the.mc_playback .fes_hilb_right));
  except_stats_reset(&(the.mc_transcode.fes_hilb_right));
 }

 if(res_sr_le)
 {
  except_stats_reset(&(the.mc_playback .fes_sr_left));
  except_stats_reset(&(the.mc_transcode.fes_sr_left));
 }

 if(res_sr_ri)
 {
  except_stats_reset(&(the.mc_playback .fes_sr_right));
  except_stats_reset(&(the.mc_transcode.fes_sr_right));
 }
}

/* get float exceptions statistics by group
*/
void fecs_getcnts(
      FP_EXCEPT_STATS *fec_hilb_le
    , FP_EXCEPT_STATS *fec_hilb_ri
    , FP_EXCEPT_STATS *fec_sr_le
    , FP_EXCEPT_STATS *fec_sr_ri
    )
{
// do not need any locks
 if(fec_hilb_le)
 {
  fec_hilb_le -> is_enable = the.mc_playback.fes_hilb_left.is_enable || the.mc_transcode.fes_hilb_left.is_enable;
  fec_hilb_le -> cnt_total = the.mc_playback.fes_hilb_left.cnt_total +  the.mc_transcode.fes_hilb_left.cnt_total;
  fec_hilb_le -> cnt_snan  = the.mc_playback.fes_hilb_left.cnt_snan  +  the.mc_transcode.fes_hilb_left.cnt_snan;
  fec_hilb_le -> cnt_qnan  = the.mc_playback.fes_hilb_left.cnt_qnan  +  the.mc_transcode.fes_hilb_left.cnt_qnan;
  fec_hilb_le -> cnt_ninf  = the.mc_playback.fes_hilb_left.cnt_ninf  +  the.mc_transcode.fes_hilb_left.cnt_ninf;
  fec_hilb_le -> cnt_nden  = the.mc_playback.fes_hilb_left.cnt_nden  +  the.mc_transcode.fes_hilb_left.cnt_nden;
  fec_hilb_le -> cnt_pden  = the.mc_playback.fes_hilb_left.cnt_pden  +  the.mc_transcode.fes_hilb_left.cnt_pden;
  fec_hilb_le -> cnt_pinf  = the.mc_playback.fes_hilb_left.cnt_pinf  +  the.mc_transcode.fes_hilb_left.cnt_pinf;
 }

 if(fec_hilb_ri)
 {
  fec_hilb_ri -> is_enable = the.mc_playback.fes_hilb_right.is_enable || the.mc_transcode.fes_hilb_right.is_enable;
  fec_hilb_ri -> cnt_total = the.mc_playback.fes_hilb_right.cnt_total +  the.mc_transcode.fes_hilb_right.cnt_total;
  fec_hilb_ri -> cnt_snan  = the.mc_playback.fes_hilb_right.cnt_snan  +  the.mc_transcode.fes_hilb_right.cnt_snan;
  fec_hilb_ri -> cnt_qnan  = the.mc_playback.fes_hilb_right.cnt_qnan  +  the.mc_transcode.fes_hilb_right.cnt_qnan;
  fec_hilb_ri -> cnt_ninf  = the.mc_playback.fes_hilb_right.cnt_ninf  +  the.mc_transcode.fes_hilb_right.cnt_ninf;
  fec_hilb_ri -> cnt_nden  = the.mc_playback.fes_hilb_right.cnt_nden  +  the.mc_transcode.fes_hilb_right.cnt_nden;
  fec_hilb_ri -> cnt_pden  = the.mc_playback.fes_hilb_right.cnt_pden  +  the.mc_transcode.fes_hilb_right.cnt_pden;
  fec_hilb_ri -> cnt_pinf  = the.mc_playback.fes_hilb_right.cnt_pinf  +  the.mc_transcode.fes_hilb_right.cnt_pinf;
 }

 if(fec_sr_le)
 {
  fec_sr_le -> is_enable = the.mc_playback.fes_sr_left.is_enable || the.mc_transcode.fes_sr_left.is_enable;
  fec_sr_le -> cnt_total = the.mc_playback.fes_sr_left.cnt_total +  the.mc_transcode.fes_sr_left.cnt_total;
  fec_sr_le -> cnt_snan  = the.mc_playback.fes_sr_left.cnt_snan  +  the.mc_transcode.fes_sr_left.cnt_snan;
  fec_sr_le -> cnt_qnan  = the.mc_playback.fes_sr_left.cnt_qnan  +  the.mc_transcode.fes_sr_left.cnt_qnan;
  fec_sr_le -> cnt_ninf  = the.mc_playback.fes_sr_left.cnt_ninf  +  the.mc_transcode.fes_sr_left.cnt_ninf;
  fec_sr_le -> cnt_nden  = the.mc_playback.fes_sr_left.cnt_nden  +  the.mc_transcode.fes_sr_left.cnt_nden;
  fec_sr_le -> cnt_pden  = the.mc_playback.fes_sr_left.cnt_pden  +  the.mc_transcode.fes_sr_left.cnt_pden;
  fec_sr_le -> cnt_pinf  = the.mc_playback.fes_sr_left.cnt_pinf  +  the.mc_transcode.fes_sr_left.cnt_pinf;
 }

 if(fec_sr_ri)
 {
  fec_sr_ri -> is_enable = the.mc_playback.fes_sr_right.is_enable || the.mc_transcode.fes_sr_right.is_enable;
  fec_sr_ri -> cnt_total = the.mc_playback.fes_sr_right.cnt_total +  the.mc_transcode.fes_sr_right.cnt_total;
  fec_sr_ri -> cnt_snan  = the.mc_playback.fes_sr_right.cnt_snan  +  the.mc_transcode.fes_sr_right.cnt_snan;
  fec_sr_ri -> cnt_qnan  = the.mc_playback.fes_sr_right.cnt_qnan  +  the.mc_transcode.fes_sr_right.cnt_qnan;
  fec_sr_ri -> cnt_ninf  = the.mc_playback.fes_sr_right.cnt_ninf  +  the.mc_transcode.fes_sr_right.cnt_ninf;
  fec_sr_ri -> cnt_nden  = the.mc_playback.fes_sr_right.cnt_nden  +  the.mc_transcode.fes_sr_right.cnt_nden;
  fec_sr_ri -> cnt_pden  = the.mc_playback.fes_sr_right.cnt_pden  +  the.mc_transcode.fes_sr_right.cnt_pden;
  fec_sr_ri -> cnt_pinf  = the.mc_playback.fes_sr_right.cnt_pinf  +  the.mc_transcode.fes_sr_right.cnt_pinf;
 }
}

/* Sound renders -- all of the
 * ----- ------- -- --- -- ---
 */
/* get current _volatile config_ sound render's parameters
*/
void srenders_get_vcfg(SR_VCONFIG *sr_cfg)
{
 // the locks really don't need here -- paranoja only
 EnterCriticalSection(&the.cs_render_change);

 sound_render_copy_cfg(&the.cfg.sr_config, sr_cfg);

 LeaveCriticalSection(&the.cs_render_change);
}

/* set _volatile config_ for _all_ sound renders
*/
void srenders_set_vcfg(const SR_VCONFIG *sr_cfg)
{
 EnterCriticalSection(&the.cs_render_change);

 sound_render_copy_cfg(sr_cfg, &the.cfg.sr_config);

 sound_render_setup(sr_cfg, &the.mc_playback.sr_left);
 sound_render_setup(sr_cfg, &the.mc_playback.sr_right);
 sound_render_setup(sr_cfg, &the.mc_transcode.sr_left);
 sound_render_setup(sr_cfg, &the.mc_transcode.sr_right);

 LeaveCriticalSection(&the.cs_render_change);
}

/* lock access to render's volatile objects
*/
void all_srenders_lock(void)
{
 EnterCriticalSection(&the.cs_render_change);
}

/* unlock access to render volatile objects
*/
void all_srenders_unlock(void)
{
 LeaveCriticalSection(&the.cs_render_change);
}


/* Very miscellaneous
 * ---- -------------
 */
/* lock mc_playback access to it's non-permanent objects
*/
void mp_playback_lock(void)
{
 EnterCriticalSection(&the.cs_playback_mc_access);
}

/* unlock mc_playback access to it's non-permanent objects
*/
void mp_playback_unlock(void)
{
 LeaveCriticalSection(&the.cs_playback_mc_access);
}

/* sleep for playback according config
*/
void playback_sleep(void)
{
 if(!the.cfg.disable_play_sleep)
  Sleep(the.cfg.play_sleep);
}

/* The general stuff
 * --- ------- -----
 */
/* main DLL function (CRT USED!)
*/
BOOL WINAPI DllMain(HANDLE hInst, DWORD reasonForCall, LPVOID lpReserved)
{
 switch(reasonForCall)
 {
  case DLL_PROCESS_ATTACH:              // load library
   // ..nothing yet..
   break;

  case DLL_PROCESS_DETACH:              // free library
   // last chance to save config and delete (partially) module object. This cleanup incomplete!!
   // FIXME::remove next two lines
   if(the.cfg.enable_unload_cleanup)
    module_cleanup();
   break;

  case DLL_THREAD_ATTACH:               // some new thread
   // ..nothing yet..
   break;

  case DLL_THREAD_DETACH:               // some thread exit cleanly
   // ..nothing yet..
   break;

  default:                              // unreachable
   return FALSE;
   break;
 }

 return TRUE;
}

/* exported symbol -- read config; returns module interface
*/
__declspec(dllexport) In_Module *winampGetInModule2(void)
{
 // initialization of 'the'
 memset(&the, 0, sizeof(the));

 // we don't need to do any preconfigugation for the.cfg.*
 (void)load_config_default();

 InitializeCriticalSection(&the.cs_hilbert_change);
 InitializeCriticalSection(&the.cs_render_change);
 InitializeCriticalSection(&the.cs_playback_mc_access);
 // the next don't need any additional initialization
 mod_context_init(&the.mc_playback);
 mod_context_init(&the.mc_transcode);

 (void)amod_init(the.cfg.dsp_list, the.cfg.is_frmod_scaled);
 // the.cfg.dsp_list now is property of amod or free()'d by it's _init()
 the.cfg.dsp_list = NULL;

 make_exts_list(the.cfg.is_wav_support, the.cfg.is_rwave_support);
 return get_playback_iface();
}

/* the whole module cleanup -- complement to winampGetInModule2()
*/
void module_cleanup(void)
{
 // can be called from DllMain as last chance to save config;
 // some objects will not free in this case
 static volatile LONG was_cleanup = 0;

 if(0 == InterlockedCompareExchange(&was_cleanup, 1, 0))
 {
  the.cfg.dsp_list = amod_cleanup(TRUE);

  mod_context_cleanup(&the.mc_playback);
  mod_context_cleanup(&the.mc_transcode);
  DeleteCriticalSection(&the.cs_playback_mc_access);
  DeleteCriticalSection(&the.cs_render_change);
  DeleteCriticalSection(&the.cs_hilbert_change);

  (void)save_config_default();
 }
}

/* the end...
*/

