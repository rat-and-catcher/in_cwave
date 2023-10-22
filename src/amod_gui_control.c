/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      amod_gui_control.c -- advanced modulator GUI setup
 *      (this file is "add-in" to gui_cwave.c)
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

// This dialog is ugly hack in attempt to made the life of it's writer easer.
// so, it's _modal_ dialog. To make it better, the DSP_NODE controls shall be
// grouped in separate child dialogs and switched by DSP-list elemet selection.
// But in this case we need non-modal dialog which has difficults to interract
// with player's windows -- especiallly, when the player closing while our
// message loop is running. So, lets stay it easy and dirty.

/* The main dialog possible sizes
 * --- ---- ------ -------- -----
 */
#define AMD_XMAX        (535)           /* width */
#define AMD_YMAX_NORM   (299)           /* normal heigth */
#define AMD_YMAX_FEC    (374)           /* heigth with FE counters */

/* The main advanced modulator control dialog
 * --- ---- -------- --------- ------- ------
 */
// Internal context of advanced modulator setup
typedef struct tagAMOD_GUI_CONTEXT
{
// setup's copy of module parameters
 NODE_DSP *lHead;                       // the head of DSP list
 NODE_DSP *lCur;                        // the current node of DSP list
 HINSTANCE loc_hi;                      // local copy of HINSTANCE
 int newnode_def_id;                    // the default ID of new DSP node
 // GUI internals
 int timer_id;                          // timer identifier
 // the shown values of the counters
 uint64_t snfr_play;                    // playback sample counter
 uint64_t snfr_trans;                   // transcode sample counter
 uint64_t snrj_play;                    // playback de-subnorm counter
 uint64_t snrj_trans;                   // transcode de-subnorm counter
 unsigned sl_cnt;                       // clips counter -- left channel
 unsigned sr_cnt;                       // clips counter -- left channel
 double sl_peak;                        // peak value -- left channel
 double sr_peak;                        // peak value -- right channel
 // the shown values of the FP exception counters
 FP_EXCEPT_STATS fec_hilb_left;         // all Hilberts -- left channel
 FP_EXCEPT_STATS fec_hilb_right;        // all Hilberts -- right channel
 FP_EXCEPT_STATS fec_sr_left;           // sound render -- left channel
 FP_EXCEPT_STATS fec_sr_right;          // sound render -- right channel
} AMOD_GUI_CONTEXT;

static const struct bus_ids             // ID's of inputs and outputs
{
 int inp_id;                            // Input ID
 int out_id;                            // Output ID
} BusIds[N_INPUTS] =
{
 { IDC_IN0, 0 },        { IDC_INA, IDC_OUTA }, { IDC_INB, IDC_OUTB },
 { IDC_INC, IDC_OUTC }, { IDC_IND, IDC_OUTD }, { IDC_INE, IDC_OUTE },
 { IDC_INF, IDC_OUTF }, { IDC_ING, IDC_OUTG }, { IDC_INH, IDC_OUTH },
 { IDC_INI, IDC_OUTI }, { IDC_INJ, IDC_OUTJ }, { IDC_INK, IDC_OUTK },
 { IDC_INL, IDC_OUTL }, { IDC_INM, IDC_OUTM }, { IDC_INN, IDC_OUTN },
 { IDC_INO, IDC_OUTO }, { IDC_INP, IDC_OUTP }, { IDC_INQ, IDC_OUTQ },
 { IDC_INR, IDC_OUTR }, { IDC_INS, IDC_OUTS }, { IDC_INT, IDC_OUTT },
 { IDC_INU, IDC_OUTU }, { IDC_INV, IDC_OUTV }, { IDC_INW, IDC_OUTW },
 { IDC_INX, IDC_OUTX }, { IDC_INY, IDC_OUTY }, { IDC_INZ, IDC_OUTZ }
};

// format for print dither depth bits
static const TCHAR fmt_dth_bits[] = _T("%.2f");

/* Forward declarations
 * ------- ------------
 */
/* the front-end for Add DSP Node dialog
*/
static NODE_DSP *add_dsp_node_dialog(HWND hwndParent, AMOD_GUI_CONTEXT *agc);
/* the front-end edit value dialog, TRUE == OK
*/
static BOOL edit_value_dialog(HWND hwndParent,  int id_par_slider,
        const TCHAR *title, double *val, AMOD_GUI_CONTEXT *agc);
/* the front-end for plugin setup gialog, TRUE==OK, FALSE==Cancel
*/
static BOOL plugin_systetup_dialog(HWND hwndParent,
        IN_CWAVE_CFG *ic_conf, AMOD_GUI_CONTEXT *agc);

/* front-end advanced modulator GUI setup
*/
void amgui_init(void)
{
 // nothing yet (already nothing!)
}

/* front-end advanced modulator GUI cleanup
*/
void amgui_cleanup(void)
{
 // nothing yet
}

/* GUI helpers
 * --- -------
 */
/* get current DSP list
*/
static void GetDspList(HWND hwnd, AMOD_GUI_CONTEXT *agc)
{
 NODE_DSP *pDsp;

 LB_ClearAll(hwnd, IDL_DSP);
 pDsp = agc -> lCur = agc -> lHead = amod_get_headdsp();
 while(pDsp)
 {
  LB_AddRecord(hwnd, IDL_DSP, pDsp -> name, (LPARAM)pDsp);
  pDsp = pDsp -> next;
 }
 LB_SetSel(hwnd, IDL_DSP, 0);
}

/* setup slider pair
*/
static void SetupSliderPair(HWND hwnd, int idl, int idr, LONG ma, LONG mi,
        LONG ls, LONG ps, LONG tk)
{
 TB_SetRange(hwnd, idl, ma, mi, TRUE);
 TB_SetSteps(hwnd, idl, ls, ps);
 TB_SetTicks(hwnd, idl, tk);

 TB_SetRange(hwnd, idr, ma, mi, TRUE);
 TB_SetSteps(hwnd, idr, ls, ps);
 TB_SetTicks(hwnd, idr, tk);
}

/* update slider's texts
*/
// -- the fuction type
typedef void (*UPDTXT_VAL)(HWND hwnd, double gainl, double gainr);
// -- the functions
static void updtxt_gain(HWND hwnd, double gainl, double gainr)
{
 TXT_PrintTxt(hwnd, IDS_GAIN,
        the.cfg.show_long_numbers?
        _T("Gain: %.14G (%.4f dB) / %.14G (%.4f dB)")
        :
        _T("Gain: %.2f (%.2f dB) / %.2f (%.2f dB)"),
        gainl, gainl > 0.0? 20.0 * log10(gainl) : -100.0,
        gainr, gainr > 0.0? 20.0 * log10(gainr) : -100.0);
}
static void updtxt_fshift(HWND hwnd, double shiftl, double shiftr)
{
 TXT_PrintTxt(hwnd, IDS_FSHIFT,
        the.cfg.show_long_numbers?
         _T("Frequency shift: %.14G / %.14G Hz")
        :
         _T("Frequency shift: %.1f / %.1f Hz"),
        amod_true_freq(shiftl), amod_true_freq(shiftr));
}
static void updtxt_pmfreq(HWND hwnd, double pmfl, double pmfr)
{
 TXT_PrintTxt(hwnd, IDS_PMFR,
        the.cfg.show_long_numbers?
        _T("Phase modulation frequency: %.14G / %.14G Hz")
        :
        _T("Phase modulation frequency: %.1f / %.1f Hz"),
        amod_true_freq(pmfl), amod_true_freq(pmfr));
}
static void updtxt_pmphase(HWND hwnd, double pmphl, double pmphr)
{
 TXT_PrintTxt(hwnd, IDS_PMIPH,
        the.cfg.show_long_numbers?
        _T("Phase modulation phase (int): %.10G (%.7G deg) / %.10G (%.7G deg)")
        :
        _T("Phase modulation phase (int): %.2f (%.1f deg) / %.2f (%.1f deg)"),
        pmphl, pmphl * 180.0, pmphr, pmphr * 180.0);
}
static void updtxt_pmlevel(HWND hwnd, double pmlevl, double pmlevr)
{
 TXT_PrintTxt(hwnd, IDS_PMLEV,
        the.cfg.show_long_numbers?
        _T("Phase modulation level: %.14G / %.14G")
        :
        _T("Phase modulation level: %.2f / %.2f"),
        pmlevl, pmlevr);
}
static void updtxt_pmangle(HWND hwnd, double pmangl, double pmangr)
{
 TXT_PrintTxt(hwnd, IDS_PMANG,
        the.cfg.show_long_numbers?
        _T("Phase modulation angle (ext): %.10G (%.7G deg) / %.10G (%.7G deg)")
        :
        _T("Phase modulation angle (ext): %.2f (%.1f deg) / %.2f (%.1f deg)"),
        pmangl, pmangl * 180.0, pmangr, pmangr * 180.0);
}

/* update slider's state
*/
// -- the function type
typedef void (*UPDBAR_VAL)(HWND hwnd, double gain, int id);
// -- the functions
static void updbar_gain(HWND hwnd, double gain, int id)
{
 // gain 0.0-MAXGAIN -> *100
 TB_SetTrack(hwnd, id, (LONG)(gain * 100.0), TRUE);
}
static void updbar_fshift(HWND hwnd, double shift, int id)
{
 // shift -MAX_FSHIFT - MAX_FSHIFT -> (+MAX_FSHIFT) * 10
  TB_SetTrack(hwnd, id, (LONG)((shift + MAX_FSHIFT) * 10.0), TRUE);
}
static void updbar_pmfreq(HWND hwnd, double pmf, int id)
{
 // PM frequency 0.0-MAX_PMFREQ -> *10
 TB_SetTrack(hwnd, id, (LONG)(pmf * 10.0), TRUE);
}
static void updbar_pmphase(HWND hwnd, double pmphase, int id)
{
 // PM angle MIN_PMPHASE-MAX_PMPHASE -> *100 + 100
 TB_SetTrack(hwnd, id, (LONG)(pmphase * 100.0 + 100.0), TRUE);
}
static void updbar_pmlevel(HWND hwnd, double pmlev, int id)
{
 // PM level 0.0-MAX_PMLEVEL -> *100
 TB_SetTrack(hwnd, id, (LONG)(pmlev * 100.0), TRUE);
}
static void updbar_pmangle(HWND hwnd, double pmang, int id)
{
 // PM angle MIN_PMANGLE-MAX_PMANGLE -> *100 + 100
 TB_SetTrack(hwnd, id, (LONG)(pmang * 100.0 + 100.0), TRUE);
}

/* handle changes of slider's state (this function use only once
 * _by definition_, so it's a good idea make them inline)
 */
static __inline int hdlbar_gain(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_GAINL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> l_gain), ((double)TB_GetTrack(hwnd, IDC_GAINL)) / 100.0);
  if(agc -> lCur -> lock_gain)
  {
   adbl_write(&(agc -> lCur -> r_gain), agc -> lCur -> l_gain);
   updbar_gain(hwnd, agc -> lCur -> r_gain, IDC_GAINR);
  }
  updtxt_gain(hwnd, agc -> lCur -> l_gain, agc -> lCur -> r_gain);
  return 1;                                     // handled
 }
 // right channel
 if(!agc -> lCur -> lock_gain && GetDlgItem(hwnd, IDC_GAINR) == hwctl)
 {
  adbl_write(&(agc -> lCur -> r_gain), ((double)TB_GetTrack(hwnd, IDC_GAINR)) / 100.0);
  updtxt_gain(hwnd, agc -> lCur -> l_gain, agc -> lCur -> r_gain);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}
static __inline int hdlbar_fshift(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_FSHIFTL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_shift.le.fr_shift),
        ((double)TB_GetTrack(hwnd, IDC_FSHIFTL)) / 10.0 - MAX_FSHIFT);
  if(agc -> lCur -> dsp.mk_shift.lock_shift)
  {
   adbl_write(&(agc -> lCur -> dsp.mk_shift.ri.fr_shift),
        agc -> lCur -> dsp.mk_shift.sign_lock_shift?
                -agc -> lCur -> dsp.mk_shift.le.fr_shift
                :
                agc -> lCur -> dsp.mk_shift.le.fr_shift);
   updbar_fshift(hwnd, agc -> lCur -> dsp.mk_shift.ri.fr_shift, IDC_FSHIFTR);
  }
  updtxt_fshift(hwnd,
        agc -> lCur -> dsp.mk_shift.le.fr_shift, agc -> lCur -> dsp.mk_shift.ri.fr_shift);
  return 1;                                     // handled
 }
 // right channel
 if(!agc -> lCur -> dsp.mk_shift.lock_shift && GetDlgItem(hwnd, IDC_FSHIFTR) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_shift.ri.fr_shift),
        ((double)TB_GetTrack(hwnd, IDC_FSHIFTR)) / 10.0 - MAX_FSHIFT);
  updtxt_fshift(hwnd,
        agc -> lCur -> dsp.mk_shift.le.fr_shift, agc -> lCur -> dsp.mk_shift.ri.fr_shift);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}
static __inline int hdlbar_pmfreq(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_PMFRL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.le.freq),
        ((double)TB_GetTrack(hwnd, IDC_PMFRL)) / 10.0);
 if(agc -> lCur -> dsp.mk_pm.lock_freq)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.freq), agc -> lCur -> dsp.mk_pm.le.freq);
  updbar_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.ri.freq, IDC_PMFRR);
 }
 updtxt_pmfreq(hwnd,
        agc -> lCur -> dsp.mk_pm.le.freq, agc -> lCur -> dsp.mk_pm.ri.freq);
 return 1;                                      // handled
 }
 // right channel
 if(!agc -> lCur -> dsp.mk_pm.lock_freq && GetDlgItem(hwnd, IDC_PMFRR) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.freq),
        ((double)TB_GetTrack(hwnd, IDC_PMFRR)) / 10.0);
  updtxt_pmfreq(hwnd,
        agc -> lCur -> dsp.mk_pm.le.freq, agc -> lCur -> dsp.mk_pm.ri.freq);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}
static __inline int hdlbar_pmphase(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_PMIPHL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.le.phase),
        ((double)TB_GetTrack(hwnd, IDC_PMIPHL)) / 100.0 - MAX_PMPHASE);
  if(agc -> lCur -> dsp.mk_pm.lock_phase)
  {
   adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.phase), agc -> lCur -> dsp.mk_pm.le.phase);
   updbar_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.ri.phase, IDC_PMIPHR);
  }
  updtxt_pmphase(hwnd,
        agc -> lCur -> dsp.mk_pm.le.phase, agc -> lCur -> dsp.mk_pm.ri.phase);
  return 1;                                     // handled
 }
 // right channel
 if(!agc -> lCur -> dsp.mk_pm.lock_phase && GetDlgItem(hwnd, IDC_PMIPHR) == hwctl)
 {
  adbl_write(&(agc ->  lCur -> dsp.mk_pm.ri.phase),
        ((double)TB_GetTrack(hwnd, IDC_PMIPHR)) / 100.0 - MAX_PMPHASE);
  updtxt_pmphase(hwnd,
        agc -> lCur -> dsp.mk_pm.le.phase, agc -> lCur -> dsp.mk_pm.ri.phase);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}
static __inline int hdlbar_pmlevel(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_PMLEVL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.le.level),
        ((double)TB_GetTrack(hwnd, IDC_PMLEVL)) / 100.0);
  if(agc -> lCur -> dsp.mk_pm.lock_level)
  {
   adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.level), agc -> lCur -> dsp.mk_pm.le.level);
   updbar_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.ri.level, IDC_PMLEVR);
  }
  updtxt_pmlevel(hwnd,
        agc -> lCur -> dsp.mk_pm.le.level, agc -> lCur -> dsp.mk_pm.ri.level);
  return 1;                                     // handled
 }
 // right channel
 if(!agc -> lCur -> dsp.mk_pm.lock_level && GetDlgItem(hwnd, IDC_PMLEVR) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.level),
        ((double)TB_GetTrack(hwnd, IDC_PMLEVR)) / 100.0);
  updtxt_pmlevel(hwnd,
        agc -> lCur -> dsp.mk_pm.le.level, agc -> lCur -> dsp.mk_pm.ri.level);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}
static __inline int hdlbar_pmangle(HWND hwnd, HWND hwctl, AMOD_GUI_CONTEXT *agc)
{
 // left channel
 if(GetDlgItem(hwnd, IDC_PMANGL) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.le.angle),
        ((double)TB_GetTrack(hwnd, IDC_PMANGL)) / 100.0 - MAX_PMANGLE);
  if(agc -> lCur -> dsp.mk_pm.lock_angle)
  {
   adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.angle), agc -> lCur -> dsp.mk_pm.le.angle);
   updbar_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.ri.angle, IDC_PMANGR);
  }
  updtxt_pmangle(hwnd,
        agc -> lCur -> dsp.mk_pm.le.angle, agc -> lCur -> dsp.mk_pm.ri.angle);
  return 1;                                     // handled
 }
 // right channel
 if(!agc -> lCur -> dsp.mk_pm.lock_angle && GetDlgItem(hwnd, IDC_PMANGR) == hwctl)
 {
  adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.angle),
        ((double)TB_GetTrack(hwnd, IDC_PMANGR)) / 100.0 - MAX_PMANGLE);
  updtxt_pmangle(hwnd,
        agc -> lCur -> dsp.mk_pm.le.angle, agc -> lCur -> dsp.mk_pm.ri.angle);
  return 1;                                     // handled
 }
 return 0;                                      // not handled
}

/* set exact "slider-based" value (odd function to minimize edit other code)
*/
static void set_exact_value(HWND hwnd,
        int id_par_slider, int id_mirror_slider, int is_mirror_lock,
        int mirror_mirror, // 0==parameter; 1==mirror only; -1==neg. mirror
        const TCHAR *title, volatile double *par_val, volatile double *mirror_val,
        double min_val, double max_val,
        UPDBAR_VAL updbar_val, UPDTXT_VAL updtxt_val, AMOD_GUI_CONTEXT *agc)
{
 double neval;
 int id_val;

 if(mirror_mirror > 0 /* mirror only */)
 {
  neval = *mirror_val;
  id_val = id_mirror_slider;
 }
 else
 {
  neval = *par_val;
  id_val = id_par_slider;
 }

 if(edit_value_dialog(hwnd, id_val, title, &neval, agc)
        && neval >= min_val
        && neval <= max_val)
 {
  adbl_write(mirror_mirror > 0? mirror_val : par_val, neval);
  (*updbar_val)(hwnd, neval, id_val);

  if(is_mirror_lock)
  {
   adbl_write(mirror_val, mirror_mirror < 0? -(*par_val) : *par_val);
   (*updbar_val)(hwnd, *mirror_val, id_mirror_slider);
  }

  (*updtxt_val)(hwnd, *par_val, *mirror_val);
 }
}

/* setup output plug
*/
static void upd_outplug(HWND hwnd, int n_out)
{
 if(n_out > 0 && n_out < N_INPUTS)
  CheckRadioButton(hwnd, IDC_OUTA, IDC_OUTZ, BusIds[n_out].out_id);
}

/* setup text for I/Q swap (spectrum inversion)
*/
static void updtxt_xiq(HWND hwnd, int inv_l, int inv_r)
{
 static const TCHAR iq_direct[] = _T("n");
 static const TCHAR iq_iverse[] = _T("i");

 SetWindowText(GetDlgItem(hwnd, IDB_XIQL), inv_l? iq_iverse : iq_direct);
 SetWindowText(GetDlgItem(hwnd, IDB_XIQR), inv_r? iq_iverse : iq_direct);
}

/* setup text for sign of locking for the fshift
*/
static void updtxt_sign_lock_shift(HWND hwnd, int sign)
{
 SetWindowText(GetDlgItem(hwnd, IDC_FSHIFT_LOCK),
        sign? _T("^") : _T("="));
}

/* disable unused and enable usable controls for each mode
*/
static void endis_mode(HWND hwnd, const int *disen_list)
{
 // disable part
 while(*disen_list)
  EnDis(hwnd, *disen_list++, FALSE);

 // enable part
 while(*++disen_list)
  EnDis(hwnd, *disen_list, TRUE);
}

/* setup combo box according string list and current index
*/
static void setup_combobox(HWND hwnd, int id, const TCHAR **list, unsigned ix)
{
 unsigned i;

 CB_ClearAll(hwnd, id);

 for(i = 0; list[i]; ++i)
 {
  CB_AddRecord(hwnd, id, list[i], (LPARAM)i);
 }
 (void)CB_SetSel(hwnd, id, ix);
}

/* setup text for quantize type button
*/
static void updtxt_quantize_type(HWND hwnd, unsigned ix)
{
 SetWindowText(GetDlgItem(hwnd, IDB_QUANTIZE),
        ix <= SND_QUANTZ_MAX? sound_render_get_quantznames()[ix] : _T("???"));
}

/* update text according to global bypass DSP-list flag
*/
static void updtxt_bypass_list_flag(HWND hwnd)
{
 BOOL is_bypass = amod_get_bypass_list_flag();

 SetWindowText(GetDlgItem(hwnd, IDC_BYPASS_LIST),
    is_bypass? _T("-!- DSP LIST BYPASSED -!-") : _T("Bypass DSP-list"));
}

/* update controls according selected DSP-node
*/
static void UpdateControls(HWND hwnd, AMOD_GUI_CONTEXT *agc)
{
 int i;

 // disable/enable lists
 static const int disen_master[] =
 {
  // -- disable:
  // outputs
  IDC_OUTA, IDC_OUTB, IDC_OUTC, IDC_OUTD, IDC_OUTE, IDC_OUTF, IDC_OUTG,
  IDC_OUTH, IDC_OUTI, IDC_OUTJ, IDC_OUTK, IDC_OUTL, IDC_OUTM, IDC_OUTN,
  IDC_OUTO, IDC_OUTP, IDC_OUTQ, IDC_OUTR, IDC_OUTS, IDC_OUTT, IDC_OUTU,
  IDC_OUTV, IDC_OUTW, IDC_OUTX, IDC_OUTY, IDC_OUTZ,
  IDS_OUTPUTS,
  // shift
  IDC_FSHIFTL, IDC_FSHIFTR, IDB_FSHIFTL, IDB_FSHIFTR,
  IDC_FSHIFTL_BYPASS, IDC_FSHIFTR_BYPASS, IDC_FSHIFT_LOCK,
  IDS_FSHIFT,
  // phase modulation
  IDC_PMFRL, IDC_PMFRR, IDB_PMFRL, IDB_PMFRR,
  IDC_PML_BYPASS, IDC_PMR_BYPASS, IDC_PMFR_LOCK,
  IDC_PMIPHL, IDC_PMIPHR, IDB_PMIPHL, IDB_PMIPHR, IDC_PMIPH_LOCK,
  IDC_PMLEVL, IDC_PMLEVR, IDB_PMLEVL, IDB_PMLEVR, IDC_PMLEV_LOCK,
  IDC_PMANGL, IDC_PMANGR, IDB_PMANGL, IDB_PMANGR, IDC_PMANG_LOCK,
  IDS_PMFR, IDS_PMIPH, IDS_PMLEV, IDS_PMANG,
  0,
  // -- enable:
  0
 };
 static const int disen_shift[] =
 {
  // -- disable:
  // phase modulation
  IDC_PMFRL, IDC_PMFRR, IDB_PMFRL, IDB_PMFRR,
  IDC_PML_BYPASS, IDC_PMR_BYPASS, IDC_PMFR_LOCK,
  IDC_PMIPHL, IDC_PMIPHR, IDB_PMIPHL, IDB_PMIPHR, IDC_PMIPH_LOCK,
  IDC_PMLEVL, IDC_PMLEVR, IDB_PMLEVL, IDB_PMLEVR, IDC_PMLEV_LOCK,
  IDC_PMANGL, IDC_PMANGR, IDB_PMANGL, IDB_PMANGR, IDC_PMANG_LOCK,
  IDS_PMFR, IDS_PMIPH, IDS_PMLEV, IDS_PMANG,
  0,
  // -- enable:
  // outputs
  IDC_OUTA, IDC_OUTB, IDC_OUTC, IDC_OUTD, IDC_OUTE, IDC_OUTF, IDC_OUTG,
  IDC_OUTH, IDC_OUTI, IDC_OUTJ, IDC_OUTK, IDC_OUTL, IDC_OUTM, IDC_OUTN,
  IDC_OUTO, IDC_OUTP, IDC_OUTQ, IDC_OUTR, IDC_OUTS, IDC_OUTT, IDC_OUTU,
  IDC_OUTV, IDC_OUTW, IDC_OUTX, IDC_OUTY, IDC_OUTZ,
  IDS_OUTPUTS,
  // shift -- IDC_FSHIFTR, IDB_FSHIFTR and IDC_FSHIFTR_BYPASS cab be omitted.
  IDC_FSHIFTL, IDC_FSHIFTR, IDB_FSHIFTL, IDB_FSHIFTR,
  IDC_FSHIFTL_BYPASS, IDC_FSHIFTR_BYPASS, IDC_FSHIFT_LOCK,
  IDS_FSHIFT,
  0
 };
 static const int disen_pm[] =
 {
  // -- disable:
  // shift
  IDC_FSHIFTL, IDC_FSHIFTR, IDB_FSHIFTL, IDB_FSHIFTR,
  IDC_FSHIFTL_BYPASS, IDC_FSHIFTR_BYPASS, IDC_FSHIFT_LOCK,
  IDS_FSHIFT,
  0,
  // -- enable:
  // outputs
  IDC_OUTA, IDC_OUTB, IDC_OUTC, IDC_OUTD, IDC_OUTE, IDC_OUTF, IDC_OUTG,
  IDC_OUTH, IDC_OUTI, IDC_OUTJ, IDC_OUTK, IDC_OUTL, IDC_OUTM, IDC_OUTN,
  IDC_OUTO, IDC_OUTP, IDC_OUTQ, IDC_OUTR, IDC_OUTS, IDC_OUTT, IDC_OUTU,
  IDC_OUTV, IDC_OUTW, IDC_OUTX, IDC_OUTY, IDC_OUTZ,
  IDS_OUTPUTS,
  // phase modulation -- IDC_PMFRR/IDB_PMFRR, IDC_PMIPHR/IDB_PMIPHR,
  // IDC_PMLEVR/IDB_PMLEVR, IDC_PMANGR/IDB_PMANGR and IDC_PMR_BYPASS can be omitted.
  IDC_PMFRL, IDC_PMFRR, IDB_PMFRL, IDB_PMFRR,
  IDC_PML_BYPASS, IDC_PMR_BYPASS, IDC_PMFR_LOCK,
  IDC_PMIPHL, IDC_PMIPHR, IDB_PMIPHL, IDB_PMIPHR, IDC_PMIPH_LOCK,
  IDC_PMLEVL, IDC_PMLEVR, IDB_PMLEVL, IDB_PMLEVR, IDC_PMLEV_LOCK,
  IDC_PMANGL, IDC_PMANGR, IDB_PMANGL, IDB_PMANGR, IDC_PMANG_LOCK,
  IDS_PMFR, IDS_PMIPH, IDS_PMLEV, IDS_PMANG,
  0
 };
 static const int disen_mix[] =
 {
  // -- disable:
  // shift
  IDC_FSHIFTL, IDC_FSHIFTR, IDB_FSHIFTL, IDB_FSHIFTR,
  IDC_FSHIFTL_BYPASS, IDC_FSHIFTR_BYPASS, IDC_FSHIFT_LOCK,
  IDS_FSHIFT,
  // phase modulation
  IDC_PMFRL, IDC_PMFRR, IDB_PMFRL, IDB_PMFRR,
  IDC_PML_BYPASS, IDC_PMR_BYPASS, IDC_PMFR_LOCK,
  IDC_PMIPHL, IDC_PMIPHR, IDB_PMIPHL, IDB_PMIPHR, IDC_PMIPH_LOCK,
  IDC_PMLEVL, IDC_PMLEVR, IDB_PMLEVL, IDB_PMLEVR, IDC_PMLEV_LOCK,
  IDC_PMANGL, IDC_PMANGR, IDB_PMANGL, IDB_PMANGR, IDC_PMANG_LOCK,
  IDS_PMFR, IDS_PMIPH, IDS_PMLEV, IDS_PMANG,
  0,
  // -- enable:
  // outputs
  IDC_OUTA, IDC_OUTB, IDC_OUTC, IDC_OUTD, IDC_OUTE, IDC_OUTF, IDC_OUTG,
  IDC_OUTH, IDC_OUTI, IDC_OUTJ, IDC_OUTK, IDC_OUTL, IDC_OUTM, IDC_OUTN,
  IDC_OUTO, IDC_OUTP, IDC_OUTQ, IDC_OUTR, IDC_OUTS, IDC_OUTT, IDC_OUTU,
  IDC_OUTV, IDC_OUTW, IDC_OUTX, IDC_OUTY, IDC_OUTZ,
  IDS_OUTPUTS,
  0
 };

 // radiobuttons lists
 // -- channels exchande:
 static const int radio_xch[] =
 {
  IDC_XCH_LR, IDC_XCH_RL, IDC_XCH_LL, IDC_XCH_RR, IDC_XCH_MIX
 };
 // -- left master:
 static const int radio_lmaster[] =
 {
  IDC_ML_RE, IDC_ML_IM, IDC_ML_SUM_REIM
 };
 // -- right master:
 static const int radio_rmaster[] =
 {
  IDC_MR_RE, IDC_MR_IM, IDC_MR_SUM_REIM
 };

 // inputs
 for(i = 0; i < N_INPUTS; ++i)
  CheckDlgButton(hwnd, BusIds[i].inp_id, agc -> lCur -> inputs[i]? BST_CHECKED : BST_UNCHECKED);

 // Gains
 TB_SetTrack(hwnd, IDC_GAINL, (LONG)(agc -> lCur -> l_gain * 100.0), TRUE);
 TB_SetTrack(hwnd, IDC_GAINR, (LONG)(agc -> lCur -> r_gain * 100.0), TRUE);
 updtxt_gain(hwnd, agc -> lCur -> l_gain, agc -> lCur -> r_gain);
 updtxt_xiq(hwnd, agc -> lCur -> l_iq_invert, agc -> lCur -> r_iq_invert);
 CheckDlgButton(hwnd, IDC_GAIN_LOCK, agc -> lCur -> lock_gain? BST_CHECKED : BST_UNCHECKED);
 EnDis(hwnd, IDB_XIQR,  !agc -> lCur -> lock_gain);
 EnDis(hwnd, IDC_GAINR, !agc -> lCur -> lock_gain);
 EnDis(hwnd, IDB_GAINR, !agc -> lCur -> lock_gain);

 // channels exchange
 CheckRadioButton(hwnd, IDC_XCH_LR, IDC_XCH_MIX, radio_xch[agc -> lCur -> xch_mode]);

 // DSP-specific
 switch(agc -> lCur -> mode)
 {
  case MODE_MASTER:
   // all masters outputs unchecked (and disabled)
   for(i = 1; i < N_INPUTS; ++i)
    CheckDlgButton(hwnd, BusIds[i].out_id, BST_UNCHECKED);

   // disable unused and enable usable ones
   endis_mode(hwnd, disen_master);

   // here DSP-specific need only for one-time initialization, sorry ;)
   CheckRadioButton(hwnd, IDC_ML_RE, IDC_ML_SUM_REIM,
        radio_lmaster[agc -> lCur -> dsp.mk_master.le.tout]);
   CheckRadioButton(hwnd, IDC_MR_RE, IDC_MR_SUM_REIM,
        radio_rmaster[agc -> lCur -> dsp.mk_master.ri.tout]);
   break;

  case MODE_SHIFT:
   // disable unused and enable usable ones
   endis_mode(hwnd, disen_shift);

   // output
   upd_outplug(hwnd, agc -> lCur -> dsp.mk_shift.n_out);

   // shift
   updbar_fshift(hwnd, agc -> lCur -> dsp.mk_shift.le.fr_shift, IDC_FSHIFTL);
   updbar_fshift(hwnd, agc -> lCur -> dsp.mk_shift.ri.fr_shift, IDC_FSHIFTR);
   updtxt_fshift(hwnd, agc -> lCur -> dsp.mk_shift.le.fr_shift,
        agc -> lCur -> dsp.mk_shift.ri.fr_shift);

   CheckDlgButton(hwnd, IDC_FSHIFT_LOCK,
        agc -> lCur -> dsp.mk_shift.lock_shift? BST_CHECKED : BST_UNCHECKED);
   updtxt_sign_lock_shift(hwnd, agc -> lCur -> dsp.mk_shift.sign_lock_shift);

   EnDis(hwnd, IDC_FSHIFTR,        !agc -> lCur -> dsp.mk_shift.lock_shift);
   EnDis(hwnd, IDB_FSHIFTR,        !agc -> lCur -> dsp.mk_shift.lock_shift);
   EnDis(hwnd, IDC_FSHIFTR_BYPASS, !agc -> lCur -> dsp.mk_shift.lock_shift);

   CheckDlgButton(hwnd, IDC_FSHIFTL_BYPASS,
        agc -> lCur -> dsp.mk_shift.le.is_shift? BST_UNCHECKED : BST_CHECKED);
   CheckDlgButton(hwnd, IDC_FSHIFTR_BYPASS,
        agc -> lCur -> dsp.mk_shift.ri.is_shift? BST_UNCHECKED : BST_CHECKED);
   break;

  case MODE_PM:
   // disable unused and enable usable ones
   endis_mode(hwnd, disen_pm);

   // output
   upd_outplug(hwnd, agc -> lCur -> dsp.mk_pm.n_out);

   // phase modulation
   updbar_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.le.freq, IDC_PMFRL);
   updbar_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.ri.freq, IDC_PMFRR);
   updtxt_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.le.freq,
        agc -> lCur -> dsp.mk_pm.ri.freq);

   CheckDlgButton(hwnd, IDC_PMFR_LOCK,
        agc -> lCur -> dsp.mk_pm.lock_freq? BST_CHECKED : BST_UNCHECKED);

   EnDis(hwnd, IDC_PMFRR,      !agc -> lCur -> dsp.mk_pm.lock_freq);
   EnDis(hwnd, IDB_PMFRR,      !agc -> lCur -> dsp.mk_pm.lock_freq);
   EnDis(hwnd, IDC_PMR_BYPASS, !agc -> lCur -> dsp.mk_pm.lock_freq);

   CheckDlgButton(hwnd, IDC_PML_BYPASS,
        agc -> lCur -> dsp.mk_pm.le.is_pm? BST_UNCHECKED : BST_CHECKED);
   CheckDlgButton(hwnd, IDC_PMR_BYPASS,
        agc -> lCur -> dsp.mk_pm.ri.is_pm? BST_UNCHECKED : BST_CHECKED);

   updbar_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.le.phase, IDC_PMIPHL);
   updbar_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.ri.phase, IDC_PMIPHR);
   updtxt_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.le.phase,
        agc -> lCur -> dsp.mk_pm.ri.phase);
   CheckDlgButton(hwnd, IDC_PMIPH_LOCK,
        agc -> lCur -> dsp.mk_pm.lock_phase? BST_CHECKED : BST_UNCHECKED);
   EnDis(hwnd, IDC_PMIPHR, !agc -> lCur -> dsp.mk_pm.lock_phase);
   EnDis(hwnd, IDB_PMIPHR, !agc -> lCur -> dsp.mk_pm.lock_phase);

   updbar_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.le.level, IDC_PMLEVL);
   updbar_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.ri.level, IDC_PMLEVR);
   updtxt_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.le.level,
        agc -> lCur -> dsp.mk_pm.ri.level);
   CheckDlgButton(hwnd, IDC_PMLEV_LOCK,
        agc -> lCur -> dsp.mk_pm.lock_level? BST_CHECKED : BST_UNCHECKED);
   EnDis(hwnd, IDC_PMLEVR, !agc -> lCur -> dsp.mk_pm.lock_level);
   EnDis(hwnd, IDB_PMLEVR, !agc -> lCur -> dsp.mk_pm.lock_level);

   updbar_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.le.angle, IDC_PMANGL);
   updbar_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.ri.angle, IDC_PMANGR);
   updtxt_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.le.angle,
        agc -> lCur -> dsp.mk_pm.ri.angle);
   CheckDlgButton(hwnd, IDC_PMANG_LOCK,
        agc -> lCur -> dsp.mk_pm.lock_angle? BST_CHECKED : BST_UNCHECKED);
   EnDis(hwnd, IDC_PMANGR, !agc -> lCur -> dsp.mk_pm.lock_angle);
   EnDis(hwnd, IDB_PMANGR, !agc -> lCur -> dsp.mk_pm.lock_angle);
   break;

  case MODE_MIX:
   // disable unused and enable usable ones
   endis_mode(hwnd, disen_mix);

   // output
   upd_outplug(hwnd, agc -> lCur -> dsp.mk_mix.n_out);
   break;
 }

 // can we delete the node
 EnDis(hwnd, IDB_DEL,
        !!(MODE_MASTER != agc -> lCur -> mode && NULL == agc -> lCur -> next));
}

/* show/reset frames counters
*/
static void ShowFrameCounters(HWND hwnd,
        BOOL isResetPlay, BOOL isResetTrans,
        BOOL isRedraw,
        AMOD_GUI_CONTEXT *agc)
{
 uint64_t n_frame;

 if(isResetPlay)
  mod_context_reset_framecnt(&(the.mc_playback));
 if(isResetTrans)
  mod_context_reset_framecnt(&(the.mc_transcode));

 n_frame = mod_context_get_framecnt(&(the.mc_playback));
 if((agc -> snfr_play != n_frame) || isRedraw)
 {
  TXT_PrintTxt(hwnd, IDS_NFR_PLAY, _T("%I64u"), agc -> snfr_play = n_frame);
 }

 n_frame = mod_context_get_framecnt(&(the.mc_transcode));
 if((agc -> snfr_trans != n_frame) || isRedraw)
 {
  TXT_PrintTxt(hwnd, IDS_NFR_TRANS, _T("%I64u"), agc -> snfr_trans = n_frame);
 }
}

/* show/reset de-subnorm counters
*/
static void ShowDesubnCounters(HWND hwnd,
        BOOL isResetPlay, BOOL isResetTrans,
        BOOL isRedraw,
        AMOD_GUI_CONTEXT *agc)
{
 uint64_t n_desub;

 if(isResetPlay)
  mod_context_reset_desubnorm_counter(&(the.mc_playback));
 if(isResetTrans)
  mod_context_reset_desubnorm_counter(&(the.mc_transcode));

 n_desub = mod_context_get_desubnorm_counter(&(the.mc_playback));
 if((agc -> snrj_play != n_desub) || isRedraw)
 {
  TXT_PrintTxt(hwnd, IDS_RJ_PLAY, _T("%I64u"), agc -> snrj_play = n_desub);
 }

 n_desub = mod_context_get_desubnorm_counter(&(the.mc_transcode));
 if((agc -> snrj_trans != n_desub) || isRedraw)
 {
  TXT_PrintTxt(hwnd, IDS_RJ_TRANS, _T("%I64u"), agc -> snrj_trans = n_desub);
 }
}

/* show/reset clips / peaks indicators
*/
static void ShowClipsPeaks(HWND hwnd, BOOL isReset, BOOL isRedraw, AMOD_GUI_CONTEXT *agc)
{
 unsigned lc, rc;
 double lpv, rpv;

 amod_get_clips_peaks(&lc, &rc, &lpv, &rpv, isReset);

 if((agc -> sl_cnt != lc) || (agc -> sl_peak != lpv) || isRedraw)
 {
  if(SR_ZERO_SIGNAL_DB == lpv)              // peak == -Inf?
  {
   TXT_PrintTxt(hwnd, IDS_CLIPSL, _T("%lu:-Inf"), lc);
  }
  else
  {
   TXT_PrintTxt(
      hwnd
    , IDS_CLIPSL
    , _T("%lu:%c%.1f")
    , lc
    , lpv < 0.0? _T('-') : _T('+')
    , fabs(lpv));
  }
  agc -> sl_cnt = lc;
  agc -> sl_peak = lpv;
 }

 if((agc -> sr_cnt != rc) || (agc -> sr_peak != rpv) || isRedraw)
 {
  if(SR_ZERO_SIGNAL_DB == rpv)              // peak == -Inf?
  {
   TXT_PrintTxt(hwnd, IDS_CLIPSR, _T("%lu:-Inf"), rc);
  }
  else
  {
   TXT_PrintTxt(
      hwnd
    , IDS_CLIPSR
    , _T("%lu:%c%.1f")
    , rc
    , rpv < 0.0? _T('-') : _T('+')
    , fabs(rpv));
  }
  agc -> sr_cnt = rc;
  agc -> sr_peak = rpv;
 }
}

/* update FP exception counters -- there is no common reset here
*/
static void ShowFPExceptions(HWND hwnd, BOOL endis, BOOL isRedraw, AMOD_GUI_CONTEXT *agc)
{
#define FEC_ONE(n_, f_, id_)                                        \
    if(agc -> n_.f_ != n_.f_ || isRedraw)                           \
     TXT_PrintTxt(hwnd, (id_), _T("%lu"), agc -> n_.f_ = n_.f_)

 if(endis)
 {
  FP_EXCEPT_STATS fec_hilb_left, fec_hilb_right, fec_sr_left, fec_sr_right;

  fecs_getcnts(&fec_hilb_left, &fec_hilb_right, &fec_sr_left, &fec_sr_right);

  FEC_ONE(fec_hilb_left, cnt_total, IDS_LH_TOT);
  FEC_ONE(fec_hilb_left, cnt_snan,  IDS_LH_SN);
  FEC_ONE(fec_hilb_left, cnt_qnan,  IDS_LH_QN);
  FEC_ONE(fec_hilb_left, cnt_ninf,  IDS_LH_NI);
  FEC_ONE(fec_hilb_left, cnt_nden,  IDS_LH_ND);
  FEC_ONE(fec_hilb_left, cnt_pden,  IDS_LH_PD);
  FEC_ONE(fec_hilb_left, cnt_pinf,  IDS_LH_PI);

  FEC_ONE(fec_sr_left, cnt_total, IDS_LS_TOT);
  FEC_ONE(fec_sr_left, cnt_snan,  IDS_LS_SN);
  FEC_ONE(fec_sr_left, cnt_qnan,  IDS_LS_QN);
  FEC_ONE(fec_sr_left, cnt_ninf,  IDS_LS_NI);
  FEC_ONE(fec_sr_left, cnt_nden,  IDS_LS_ND);
  FEC_ONE(fec_sr_left, cnt_pden,  IDS_LS_PD);
  FEC_ONE(fec_sr_left, cnt_pinf,  IDS_LS_PI);

  FEC_ONE(fec_hilb_right, cnt_total, IDS_RH_TOT);
  FEC_ONE(fec_hilb_right, cnt_snan,  IDS_RH_SN);
  FEC_ONE(fec_hilb_right, cnt_qnan,  IDS_RH_QN);
  FEC_ONE(fec_hilb_right, cnt_ninf,  IDS_RH_NI);
  FEC_ONE(fec_hilb_right, cnt_nden,  IDS_RH_ND);
  FEC_ONE(fec_hilb_right, cnt_pden,  IDS_RH_PD);
  FEC_ONE(fec_hilb_right, cnt_pinf,  IDS_RH_PI);

  FEC_ONE(fec_sr_right, cnt_total, IDS_RS_TOT);
  FEC_ONE(fec_sr_right, cnt_snan,  IDS_RS_SN);
  FEC_ONE(fec_sr_right, cnt_qnan,  IDS_RS_QN);
  FEC_ONE(fec_sr_right, cnt_ninf,  IDS_RS_NI);
  FEC_ONE(fec_sr_right, cnt_nden,  IDS_RS_ND);
  FEC_ONE(fec_sr_right, cnt_pden,  IDS_RS_PD);
  FEC_ONE(fec_sr_right, cnt_pinf,  IDS_RS_PI);
 }
#undef FEC_ONE
}

/* show / hide FP exceptions counters
*/
static void ShowHideExceptions(HWND hwnd, BOOL new_endis, AMOD_GUI_CONTEXT *agc)
{
 RECT rect;

 rect.left = 0;
 rect.top = 0;
 rect.right = AMD_XMAX + 1;
 rect.bottom = (new_endis? AMD_YMAX_FEC : AMD_YMAX_NORM) + 1;

 MapDialogRect(hwnd, &rect);
 AdjustWindowRect(&rect, GetWindowLong(hwnd, GWL_STYLE), FALSE);

 SetWindowPos(hwnd
    , HWND_TOP
    , 0
    , 0
    , rect.right - rect.left + 1
    , rect.bottom - rect.top + 1
    , SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

 fecs_set_endis_all(new_endis);
 ShowFPExceptions(hwnd, new_endis, TRUE, agc);
}

/* The advanced setup dialog
 * --- -------- ----- ------
 */
// Message Handlers
// ------- --------
/* dialog initialization
*/
static BOOL Setup_Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
 AMOD_GUI_CONTEXT *agc = (AMOD_GUI_CONTEXT *)lParam;
 SR_VCONFIG sr_vcfg;

 SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)agc);

 // DSP List Box
 GetDspList(hwnd, agc);

 // sliders setup
 SetupSliderPair(hwnd, IDC_GAINL, IDC_GAINR,
        (LONG)(MAX_GAIN * 100.0) /*200% max*/, 0, 5, 10, 10);
 SetupSliderPair(hwnd, IDC_FSHIFTL, IDC_FSHIFTR,
        (LONG)(MAX_FSHIFT * 20.0), 0, 1 /*0.1Hz*/, 10 /*1.0Hz*/, 20 /*2.0Hz*/);
 SetupSliderPair(hwnd, IDC_PMFRL, IDC_PMFRR,
        (LONG)(MAX_PMFREQ * 10.0), 0, 1 /*0.1Hz*/, 10 /*1.0Hz*/, 10 /*1.0Hz*/);
 SetupSliderPair(hwnd, IDC_PMIPHL, IDC_PMIPHR,
        (LONG)((MAX_PMPHASE - MIN_PMPHASE) * 100.0), 0, 1, 10, 10);
 SetupSliderPair(hwnd, IDC_PMLEVL, IDC_PMLEVR,
        (LONG)(MAX_PMLEVEL * 100.0), 0, 1, 10, 5);
 SetupSliderPair(hwnd, IDC_PMANGL, IDC_PMANGR,
        (LONG)((MAX_PMANGLE - MIN_PMANGLE) * 100.0), 0, 1, 10, 10);

 // move DSP-specific controls to defaults
 updbar_fshift(hwnd,  DEF_FSHIFT, IDC_FSHIFTL);
 updbar_fshift(hwnd, -DEF_FSHIFT, IDC_FSHIFTR);
 updtxt_fshift(hwnd,  DEF_FSHIFT, -DEF_FSHIFT);
 CheckDlgButton(hwnd, IDC_FSHIFT_LOCK, BST_CHECKED);
 updtxt_sign_lock_shift(hwnd, 1);
 EnDis(hwnd, IDC_FSHIFTR,        0);
 EnDis(hwnd, IDC_FSHIFTR_BYPASS, 0);

 updbar_pmfreq(hwnd, DEF_PMFREQ, IDC_PMFRL);
 updbar_pmfreq(hwnd, DEF_PMFREQ, IDC_PMFRR);
 updtxt_pmfreq(hwnd, DEF_PMFREQ, DEF_PMFREQ);
 CheckDlgButton(hwnd, IDC_PMFR_LOCK, BST_CHECKED);
 EnDis(hwnd, IDC_PMFRR,      0);
 EnDis(hwnd, IDC_PMR_BYPASS, 0);

 updbar_pmphase(hwnd, 0.0,         IDC_PMIPHL);
 updbar_pmphase(hwnd, DEF_PMPHASE, IDC_PMIPHR);
 updtxt_pmphase(hwnd, 0.0, DEF_PMPHASE);
 if(0.0 == DEF_PMPHASE)                         // `#if` impossible, because double type of the expr.
 {
  CheckDlgButton(hwnd, IDC_PMIPH_LOCK, BST_CHECKED);
  EnDis(hwnd, IDC_PMIPHR, 0);
 }

 updbar_pmlevel(hwnd, DEF_PMLEVEL, IDC_PMLEVL);
 updbar_pmlevel(hwnd, DEF_PMLEVEL, IDC_PMLEVR);
 updtxt_pmlevel(hwnd, DEF_PMLEVEL, DEF_PMLEVEL);
 CheckDlgButton(hwnd, IDC_PMLEV_LOCK, BST_CHECKED);
 EnDis(hwnd, IDC_PMLEVR, 0);

 updbar_pmangle(hwnd, 0.0,         IDC_PMANGL);
 updbar_pmangle(hwnd, DEF_PMANGLE, IDC_PMANGR);
 updtxt_pmangle(hwnd, 0.0, DEF_PMANGLE);
 if(0.0 == DEF_PMANGLE)                         // `#if` impossible, because double type of the expr.
 {
  CheckDlgButton(hwnd, IDC_PMANG_LOCK, BST_CHECKED);
  EnDis(hwnd, IDC_PMANGR, 0);
 }

 // update controls for current list element
 UpdateControls(hwnd, agc);

 // Hilbert
 setup_combobox(hwnd, IDL_EHILBERT,  hq_get_type_names(), the.cfg.iir_filter_no);

 // sound render
 srenders_get_vcfg(&sr_vcfg);
 setup_combobox(hwnd, IDL_RENDER, sound_render_get_rtypenames(), sr_vcfg.render_type);
 updtxt_quantize_type(hwnd, sr_vcfg.quantz_type);
 TXT_PrintTxt(hwnd, IDC_DTH_BITS, fmt_dth_bits, sr_vcfg.dth_bits);
 setup_combobox(hwnd, IDL_NOISE_SHAPING, sound_render_get_nshapenames(), sr_vcfg.nshape_type);

 // common window state
 CheckDlgButton(hwnd, IDC_BYPASS_LIST, amod_get_bypass_list_flag()? BST_CHECKED : BST_UNCHECKED);
 updtxt_bypass_list_flag(hwnd);
 CheckDlgButton(hwnd, IDC_LONG_NO,     the.cfg.show_long_numbers  ? BST_CHECKED : BST_UNCHECKED);
 CheckDlgButton(hwnd, IDC_RESET_NFR,   the.cfg.is_clr_nframe_trk  ? BST_CHECKED : BST_UNCHECKED);
 CheckDlgButton(hwnd, IDC_RESET_HILB,  the.cfg.is_clr_hilb_trk    ? BST_CHECKED : BST_UNCHECKED);
 CheckDlgButton(hwnd, IDC_ADV24BITS,   the.cfg.need24bits         ? BST_CHECKED : BST_UNCHECKED);
 CheckDlgButton(hwnd, IDC_SHOWPLAY,    getShowPlay()              ? BST_CHECKED : BST_UNCHECKED);
 CheckDlgButton(hwnd, IDC_SEXCP,       the.cfg.is_fp_check        ? BST_CHECKED : BST_UNCHECKED);


 ShowFrameCounters(hwnd, FALSE, FALSE, TRUE, agc);
 ShowDesubnCounters(hwnd, FALSE, FALSE, TRUE, agc);
 ShowClipsPeaks(hwnd, FALSE, TRUE, agc);

 ShowHideExceptions(hwnd, the.cfg.is_fp_check, agc);
 ShowFPExceptions(hwnd, the.cfg.is_fp_check, TRUE, agc);

 // create the timer 200 ms
 agc -> timer_id = SetTimer(hwnd, 308 /* ID */, 200 /* ms */, (TIMERPROC)NULL);
 return TRUE;                   // default focus
}

/* command handling
*/
// "useful" macros
// -- handle command from the inputs selectors
#define HANDLE_INPUT(ci, n)                                                                     \
  case IDC_IN##ci:                                                                              \
   agc -> lCur -> inputs[n] = !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_IN##ci));         \
   break
// -- handle command from the output selectors
#define HANDLE_OUTPUT(ci, n)                                                                    \
  case IDC_OUT##ci:                                                                             \
   amod_set_output_plug(agc -> lCur, n);                                                        \
   break

static void Setup_Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
 AMOD_GUI_CONTEXT *agc = (AMOD_GUI_CONTEXT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
 NODE_DSP *temp_node;
 SR_VCONFIG sr_vcfg;

 switch(id)
 {
  // DSP list manipulation
  case IDL_DSP:
   if(LBN_SELCHANGE == codeNotify)
   {
    LB_GetIndexRec(hwnd, IDL_DSP, (LRESULT *)(&agc -> lCur));
    UpdateControls(hwnd, agc);
   }
   break;

  // create / delete DSP node(s)
  case IDB_ADD:                         // create new node
   temp_node = add_dsp_node_dialog(hwnd, agc);
   if(INVALID_NODE_DSP != temp_node)
   {
    if(NULL == temp_node)
    {
     MessageBox(hwnd, _T("Node Creation Failed!"), _T("Internal error"),
        MB_OK | MB_ICONSTOP);
    }
    else
    {
     agc -> lCur = temp_node;
     LB_AddRecord(hwnd, IDL_DSP, agc -> lCur -> name, (LPARAM)agc -> lCur);
     UpdateControls(hwnd, agc);
    }
   }
   break;

  case IDB_DEL:                         // delete the last node
   amod_del_lastdsp();
   LB_DelRecord(hwnd, IDL_DSP);
   LB_GetIndexRec(hwnd, IDL_DSP, (LRESULT *)(&agc -> lCur));
   UpdateControls(hwnd, agc);
   break;

  case IDB_DELALL:                      // delete all user's nodes
   amod_del_dsplist();
   GetDspList(hwnd, agc);
   UpdateControls(hwnd, agc);
   break;

  // global DSP-list option(s)
  case IDC_BYPASS_LIST:                 // DSP-list global bypass
   amod_set_bypass_list_flag(!(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_BYPASS_LIST)));
   updtxt_bypass_list_flag(hwnd);
   break;

  // "local" bypasses (here and latter we don't check lCur.mode -- we should trust the dialog logic)
  case IDC_FSHIFTL_BYPASS:              // bypass shift - left
   agc -> lCur -> dsp.mk_shift.le.is_shift =
        !(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_FSHIFTL_BYPASS));
   if(agc -> lCur -> dsp.mk_shift.lock_shift) // the right bypass is same, if shift locked
   {
    CheckDlgButton(hwnd,
        IDC_FSHIFTR_BYPASS,
        (agc -> lCur -> dsp.mk_shift.ri.is_shift = agc -> lCur -> dsp.mk_shift.le.is_shift) != 0?
                BST_UNCHECKED
                :
                BST_CHECKED);
   }
   break;

  case IDC_FSHIFTR_BYPASS:              // bypass shift - right
   agc -> lCur -> dsp.mk_shift.ri.is_shift =
        !(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_FSHIFTR_BYPASS));
   break;

  case IDC_PML_BYPASS:                  // bypass PM - left
   agc -> lCur -> dsp.mk_pm.le.is_pm =
        !(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_PML_BYPASS));
   if(agc -> lCur -> dsp.mk_pm.lock_freq) // the right bypass is same, if PM freq is locked
   {
    CheckDlgButton(hwnd,
        IDC_PMR_BYPASS,
        (agc -> lCur -> dsp.mk_pm.ri.is_pm = agc -> lCur -> dsp.mk_pm.le.is_pm) != 0?
                BST_UNCHECKED
                :
                BST_CHECKED);
   }
   break;

  case IDC_PMR_BYPASS:                  // bypass PM - right
   agc -> lCur -> dsp.mk_pm.ri.is_pm =
        !(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_PMR_BYPASS));
   break;

  // locks controls
  case IDC_GAIN_LOCK:                   // lock gains
   agc -> lCur -> lock_gain = !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_GAIN_LOCK));
   EnDis(hwnd, IDB_XIQR,  !agc -> lCur -> lock_gain);
   EnDis(hwnd, IDC_GAINR, !agc -> lCur -> lock_gain);
   EnDis(hwnd, IDB_GAINR, !agc -> lCur -> lock_gain);
   if(agc -> lCur -> lock_gain)
   {
    adbl_write(&(agc -> lCur -> r_gain), agc -> lCur -> l_gain);
    updbar_gain(hwnd, agc -> lCur -> r_gain, IDC_GAINR);
    updtxt_gain(hwnd, agc -> lCur -> l_gain, agc -> lCur -> r_gain);
    updtxt_xiq (hwnd,
        agc -> lCur -> l_iq_invert,
        agc -> lCur -> r_iq_invert = agc -> lCur -> l_iq_invert);
   }
   break;

  case IDC_FSHIFT_LOCK:                 // lock spectrum shifts
   agc -> lCur -> dsp.mk_shift.lock_shift =
        !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_FSHIFT_LOCK));
   EnDis(hwnd, IDC_FSHIFTR,        !agc -> lCur -> dsp.mk_shift.lock_shift);
   EnDis(hwnd, IDB_FSHIFTR,        !agc -> lCur -> dsp.mk_shift.lock_shift);
   EnDis(hwnd, IDC_FSHIFTR_BYPASS, !agc -> lCur -> dsp.mk_shift.lock_shift);

   if(agc -> lCur -> dsp.mk_shift.lock_shift)
   {
    adbl_write(&(agc -> lCur -> dsp.mk_shift.ri.fr_shift),
        agc -> lCur -> dsp.mk_shift.sign_lock_shift?
                -agc -> lCur -> dsp.mk_shift.le.fr_shift
                :
                agc -> lCur -> dsp.mk_shift.le.fr_shift);
    updbar_fshift(hwnd, agc -> lCur -> dsp.mk_shift.ri.fr_shift, IDC_FSHIFTR);
    updtxt_fshift(hwnd, agc -> lCur -> dsp.mk_shift.le.fr_shift,
        agc -> lCur -> dsp.mk_shift.ri.fr_shift);

    CheckDlgButton(hwnd,
        IDC_FSHIFTR_BYPASS,
        (agc -> lCur -> dsp.mk_shift.ri.is_shift = agc -> lCur -> dsp.mk_shift.le.is_shift) != 0?
                BST_UNCHECKED
                :
                BST_CHECKED);
   }
   else                                 // change sign ("mirror policy") of locking spectrum shift
   {
    updtxt_sign_lock_shift(hwnd,
        agc -> lCur -> dsp.mk_shift.sign_lock_shift =
        !(agc -> lCur -> dsp.mk_shift.sign_lock_shift));
    // @here we in "unlocked" state, so wi don't need to update shift controls / indicators
   }
   break;

  case IDC_PMFR_LOCK:                   // lock PM frequency
   agc -> lCur -> dsp.mk_pm.lock_freq =
        !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_PMFR_LOCK));
   EnDis(hwnd, IDC_PMFRR,      !agc -> lCur -> dsp.mk_pm.lock_freq);
   EnDis(hwnd, IDB_PMFRR,      !agc -> lCur -> dsp.mk_pm.lock_freq);
   EnDis(hwnd, IDC_PMR_BYPASS, !agc -> lCur -> dsp.mk_pm.lock_freq);

   if(agc -> lCur -> dsp.mk_pm.lock_freq)
   {
    adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.freq), agc -> lCur -> dsp.mk_pm.le.freq);
    updbar_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.ri.freq, IDC_PMFRR);
    updtxt_pmfreq(hwnd, agc -> lCur -> dsp.mk_pm.le.freq, agc -> lCur -> dsp.mk_pm.ri.freq);

    CheckDlgButton(hwnd,                // also lock PM bypass
        IDC_PMR_BYPASS,
        (agc -> lCur -> dsp.mk_pm.ri.is_pm = agc -> lCur -> dsp.mk_pm.le.is_pm) != 0?
                BST_UNCHECKED
                :
                BST_CHECKED);
   }
   break;

  case IDC_PMIPH_LOCK:                  // lock PM phases (int)
   agc -> lCur -> dsp.mk_pm.lock_phase =
        !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_PMIPH_LOCK));
   EnDis(hwnd, IDC_PMIPHR, !agc -> lCur -> dsp.mk_pm.lock_phase);
   EnDis(hwnd, IDB_PMIPHR, !agc -> lCur -> dsp.mk_pm.lock_phase);
   if(agc -> lCur -> dsp.mk_pm.lock_phase)
   {
    adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.phase), agc -> lCur -> dsp.mk_pm.le.phase);
    updbar_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.ri.phase, IDC_PMIPHR);
    updtxt_pmphase(hwnd, agc -> lCur -> dsp.mk_pm.le.phase, agc -> lCur -> dsp.mk_pm.ri.phase);
   }
   break;

  case IDC_PMLEV_LOCK:                  // lock PM levels
   agc -> lCur -> dsp.mk_pm.lock_level =
        !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_PMLEV_LOCK));
   EnDis(hwnd, IDC_PMLEVR, !agc -> lCur -> dsp.mk_pm.lock_level);
   EnDis(hwnd, IDB_PMLEVR, !agc -> lCur -> dsp.mk_pm.lock_level);
   if(agc -> lCur -> dsp.mk_pm.lock_level)
   {
    adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.level), agc -> lCur -> dsp.mk_pm.le.level);
    updbar_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.ri.level, IDC_PMLEVR);
    updtxt_pmlevel(hwnd, agc -> lCur -> dsp.mk_pm.le.level, agc -> lCur -> dsp.mk_pm.ri.level);
   }
   break;

  case IDC_PMANG_LOCK:                  // lock PM angles (ext)
   agc -> lCur -> dsp.mk_pm.lock_angle =
        !(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_PMANG_LOCK));
   EnDis(hwnd, IDC_PMANGR, !agc -> lCur -> dsp.mk_pm.lock_angle);
   EnDis(hwnd, IDB_PMANGR, !agc -> lCur -> dsp.mk_pm.lock_angle);
   if(agc -> lCur -> dsp.mk_pm.lock_angle)
   {
    adbl_write(&(agc -> lCur -> dsp.mk_pm.ri.angle), agc -> lCur -> dsp.mk_pm.le.angle);
    updbar_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.ri.angle, IDC_PMANGR);
    updtxt_pmangle(hwnd, agc -> lCur -> dsp.mk_pm.le.angle, agc -> lCur -> dsp.mk_pm.ri.angle);
   }
   break;

  // exact slider values
  case IDB_GAINL:
   set_exact_value(hwnd, IDC_GAINL, IDC_GAINR, agc -> lCur -> lock_gain, 0,
        _T("Gain:Left"), &(agc -> lCur -> l_gain), &(agc -> lCur -> r_gain),
        0.0, MAX_GAIN, updbar_gain, updtxt_gain, agc);
   break;

  case IDB_GAINR:
   set_exact_value(hwnd, IDC_GAINL, IDC_GAINR, 0, 1,            // set mirror only
        _T("Gain:Right"), &(agc -> lCur -> l_gain), &(agc -> lCur -> r_gain),
        0.0, MAX_GAIN, updbar_gain, updtxt_gain, agc);
   break;

  case IDB_FSHIFTL:
   set_exact_value(hwnd, IDC_FSHIFTL, IDC_FSHIFTR, agc -> lCur -> dsp.mk_shift.lock_shift,
        agc -> lCur->dsp.mk_shift.sign_lock_shift? -1 : 0,      // mirror-mirror...
        _T("Freq Shift:Left"),
        &(agc -> lCur -> dsp.mk_shift.le.fr_shift), &(agc -> lCur -> dsp.mk_shift.ri.fr_shift),
        -MAX_FSHIFT, MAX_FSHIFT, updbar_fshift, updtxt_fshift, agc);
   break;

  case IDB_FSHIFTR:
   set_exact_value(hwnd, IDC_FSHIFTL, IDC_FSHIFTR, 0, 1,        // set mirror only
        _T("Freq Shift:Right"),
        &(agc -> lCur -> dsp.mk_shift.le.fr_shift), &(agc -> lCur -> dsp.mk_shift.ri.fr_shift),
        -MAX_FSHIFT, MAX_FSHIFT, updbar_fshift, updtxt_fshift, agc);
   break;

  case IDB_PMFRL:
   set_exact_value(hwnd, IDC_PMFRL, IDC_PMFRR, agc -> lCur -> dsp.mk_pm.lock_freq, 0,
        _T("PM Freq:Left"),
        &(agc -> lCur -> dsp.mk_pm.le.freq), &(agc -> lCur -> dsp.mk_pm.ri.freq),
        0, MAX_PMFREQ, updbar_pmfreq, updtxt_pmfreq, agc);
   break;

  case IDB_PMFRR:
   set_exact_value(hwnd, IDC_PMFRL, IDC_PMFRR, 0, 1,            // set mirror only
        _T("PM Freq:Right"),
        &(agc -> lCur -> dsp.mk_pm.le.freq), &(agc -> lCur -> dsp.mk_pm.ri.freq),
        0, MAX_PMFREQ, updbar_pmfreq, updtxt_pmfreq, agc);
   break;

  case IDB_PMIPHL:
   set_exact_value(hwnd, IDC_PMIPHL, IDC_PMIPHR, agc -> lCur -> dsp.mk_pm.lock_phase, 0,
        _T("PM Phase(int):Left"),
        &(agc -> lCur -> dsp.mk_pm.le.phase), &(agc -> lCur -> dsp.mk_pm.ri.phase),
        MIN_PMPHASE, MAX_PMPHASE, updbar_pmphase, updtxt_pmphase, agc);
   break;

  case IDB_PMIPHR:
   set_exact_value(hwnd, IDC_PMIPHL, IDC_PMIPHR, 0, 1,          // set mirror only
        _T("PM Phase(int):Right"),
        &(agc -> lCur -> dsp.mk_pm.le.phase), &(agc -> lCur -> dsp.mk_pm.ri.phase),
        MIN_PMPHASE, MAX_PMPHASE, updbar_pmphase, updtxt_pmphase, agc);
   break;

  case IDB_PMLEVL:
   set_exact_value(hwnd, IDC_PMLEVL, IDC_PMLEVR, agc -> lCur -> dsp.mk_pm.lock_level, 0,
        _T("PM Level:Left"),
        &(agc -> lCur -> dsp.mk_pm.le.level), &(agc -> lCur -> dsp.mk_pm.ri.level),
        0.0, MAX_PMLEVEL, updbar_pmlevel, updtxt_pmlevel, agc);
   break;

  case IDB_PMLEVR:
   set_exact_value(hwnd, IDC_PMLEVL, IDC_PMLEVR, 0, 1,          // set mirror only
        _T("PM Level:Right"),
        &(agc -> lCur -> dsp.mk_pm.le.level), &(agc -> lCur -> dsp.mk_pm.ri.level),
        0.0, MAX_PMLEVEL, updbar_pmlevel, updtxt_pmlevel, agc);
   break;

  case IDB_PMANGL:
   set_exact_value(hwnd, IDC_PMANGL, IDC_PMANGR, agc -> lCur -> dsp.mk_pm.lock_angle, 0,
        _T("PM Angle(ext):Left"),
        &(agc -> lCur -> dsp.mk_pm.le.angle), &(agc -> lCur -> dsp.mk_pm.ri.angle),
        MIN_PMANGLE, MAX_PMANGLE, updbar_pmangle, updtxt_pmangle, agc);
   break;

  case IDB_PMANGR:
   set_exact_value(hwnd, IDC_PMANGL, IDC_PMANGR, 0, 1,          // set mirror only
        _T("PM Angle(ext):Left"),
        &(agc -> lCur -> dsp.mk_pm.le.angle), &(agc -> lCur -> dsp.mk_pm.ri.angle),
        MIN_PMANGLE, MAX_PMANGLE, updbar_pmangle, updtxt_pmangle, agc);
   break;

  // I/Q exchange -- spectrum inversion
  case IDB_XIQL:
   agc -> lCur -> l_iq_invert = !agc -> lCur -> l_iq_invert;
   if(agc -> lCur -> lock_gain)
    agc -> lCur -> r_iq_invert = agc -> lCur -> l_iq_invert;
   updtxt_xiq(hwnd, agc -> lCur -> l_iq_invert, agc -> lCur -> r_iq_invert);
   break;

  case IDB_XIQR:
   agc -> lCur -> r_iq_invert = !agc -> lCur -> r_iq_invert;
   updtxt_xiq(hwnd, agc -> lCur -> l_iq_invert, agc -> lCur -> r_iq_invert);
   break;

  // Channels exchange
  case IDC_XCH_LR:
   agc -> lCur -> xch_mode = XCH_NORMAL;
   break;

  case IDC_XCH_RL:
   agc -> lCur -> xch_mode = XCH_SWAP;
   break;

  case IDC_XCH_LL:
   agc -> lCur -> xch_mode = XCH_LEFTONLY;
   break;

  case IDC_XCH_RR:
   agc -> lCur -> xch_mode = XCH_RIGHTONLY;
   break;

  case IDC_XCH_MIX:
   agc -> lCur -> xch_mode = XCH_MIXLR;
   break;

  // Master Real/Imaginary/Middle
  case IDC_ML_RE:                       // left = real
   agc -> lHead -> dsp.mk_master.le.tout = S_RE;
   break;

  case IDC_ML_IM:                       // left = imaginary
   agc -> lHead -> dsp.mk_master.le.tout = S_IM;
   break;

  case IDC_ML_SUM_REIM:                 // left = (Re + Im) / 2
   agc -> lHead -> dsp.mk_master.le.tout = S_SUM_REIM;
   break;

  case IDC_MR_RE:                       // right = real
   agc -> lHead -> dsp.mk_master.ri.tout = S_RE;
   break;

  case IDC_MR_IM:                       // right = imaginary
   agc -> lHead -> dsp.mk_master.ri.tout = S_IM;
   break;

  case IDC_MR_SUM_REIM:                 // right = (Re + Im) / 2
   agc -> lHead -> dsp.mk_master.ri.tout = S_SUM_REIM;
   break;

  // clear frame counters
  case IDB_RESET_NFR_PLAY:              // clear playback frame counter
   ShowFrameCounters(hwnd, TRUE, FALSE, TRUE, agc);
   break;

  case IDB_RESET_NFR_TRANS:             // clear transcode frame counter
   ShowFrameCounters(hwnd, FALSE, TRUE, TRUE, agc);
   break;

  // reset Hilbert transformers
  case IDB_RESET_HILB_PLAY:             // clear playback Hilbert's converter
   mod_context_reset_hilbert(&the.mc_playback);
   break;

  case IDB_RESET_HILB_TRANS:            // clear transcode Hilbert's converter
   mod_context_reset_hilbert(&the.mc_transcode);
   break;

  // clear desubnorm counters
  case IDB_RESET_RJ_PLAY:               // clear playback de-subnorm counters
   ShowDesubnCounters(hwnd, TRUE, FALSE, TRUE, agc);
   break;

  case IDB_RESET_RJ_TRANS:              // clear transcode de-subnorm counters
   ShowDesubnCounters(hwnd, FALSE, TRUE, TRUE, agc);
   break;

  // clear clips counters
  case IDB_RESET_CLIPS:
   ShowClipsPeaks(hwnd, TRUE, TRUE, agc);
   break;

  // setup clear frame counter / analitic transformers per each track
  case IDC_RESET_NFR:                   // clear frame counter per each track
   the.cfg.is_clr_nframe_trk = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RESET_NFR));
   break;

  case IDC_RESET_HILB:                  // clear Hilbert's converter per each track
   the.cfg.is_clr_hilb_trk = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RESET_HILB));
   break;

  // the inputs (so many...^)
  HANDLE_INPUT(0,  0); HANDLE_INPUT(A,  1); HANDLE_INPUT(B,  2);
  HANDLE_INPUT(C,  3); HANDLE_INPUT(D,  4); HANDLE_INPUT(E,  5);
  HANDLE_INPUT(F,  6); HANDLE_INPUT(G,  7); HANDLE_INPUT(H,  8);
  HANDLE_INPUT(I,  9); HANDLE_INPUT(J, 10); HANDLE_INPUT(K, 11);
  HANDLE_INPUT(L, 12); HANDLE_INPUT(M, 13); HANDLE_INPUT(N, 14);
  HANDLE_INPUT(O, 15); HANDLE_INPUT(P, 16); HANDLE_INPUT(Q, 17);
  HANDLE_INPUT(R, 18); HANDLE_INPUT(S, 19); HANDLE_INPUT(T, 20);
  HANDLE_INPUT(U, 21); HANDLE_INPUT(V, 22); HANDLE_INPUT(W, 23);
  HANDLE_INPUT(X, 24); HANDLE_INPUT(Y, 25); HANDLE_INPUT(Z, 26);

  // the outputs (so many...^)
  HANDLE_OUTPUT(A,  1); HANDLE_OUTPUT(B,  2); HANDLE_OUTPUT(C,  3);
  HANDLE_OUTPUT(D,  4); HANDLE_OUTPUT(E,  5); HANDLE_OUTPUT(F,  6);
  HANDLE_OUTPUT(G,  7); HANDLE_OUTPUT(H,  8); HANDLE_OUTPUT(I,  9);
  HANDLE_OUTPUT(J, 10); HANDLE_OUTPUT(K, 11); HANDLE_OUTPUT(L, 12);
  HANDLE_OUTPUT(M, 13); HANDLE_OUTPUT(N, 14); HANDLE_OUTPUT(O, 15);
  HANDLE_OUTPUT(P, 16); HANDLE_OUTPUT(Q, 17); HANDLE_OUTPUT(R, 18);
  HANDLE_OUTPUT(S, 19); HANDLE_OUTPUT(T, 20); HANDLE_OUTPUT(U, 21);
  HANDLE_OUTPUT(V, 22); HANDLE_OUTPUT(W, 23); HANDLE_OUTPUT(X, 24);
  HANDLE_OUTPUT(Y, 25); HANDLE_OUTPUT(Z, 26);

  // Hilbert's converter type
  case IDL_EHILBERT:
   if(CBN_SELCHANGE == codeNotify)
   {
    LRESULT hq_ix;

    CB_GetIndexRec(hwnd, IDL_EHILBERT, &hq_ix);
    mod_context_change_all_hilberts_filter((unsigned)hq_ix);
   }
   break;

  // Sound render type manipulation
  case IDB_DTH_BITS:                    // bits to dither
   srenders_get_vcfg(&sr_vcfg);
   sr_vcfg.dth_bits =
        TXT_GetDbl(hwnd, IDC_DTH_BITS, sr_vcfg.dth_bits, 0.0, MAX_DITHER_BITS, fmt_dth_bits);
   srenders_set_vcfg(&sr_vcfg);
   break;

  case IDB_QUANTIZE:                    // quantize type
   srenders_get_vcfg(&sr_vcfg);
   switch(sr_vcfg.quantz_type)
   {
    case SND_QUANTZ_MID_TREAD:
     sr_vcfg.quantz_type = SND_QUANTZ_MID_RISER;
     break;
    case SND_QUANTZ_MID_RISER:
     sr_vcfg.quantz_type = SND_QUANTZ_MID_TREAD;
     break;
   }
   updtxt_quantize_type(hwnd, sr_vcfg.quantz_type);
   srenders_set_vcfg(&sr_vcfg);
   break;

  case IDL_RENDER:                      // render type
   if(CBN_SELCHANGE == codeNotify)
   {
    LRESULT rend_ix;

    CB_GetIndexRec(hwnd, IDL_RENDER, &rend_ix);
    srenders_get_vcfg(&sr_vcfg);
    sr_vcfg.render_type = rend_ix;
    srenders_set_vcfg(&sr_vcfg);
   }
   break;

  case IDL_NOISE_SHAPING:               // noise shaping type
   if(CBN_SELCHANGE == codeNotify)
   {
    LRESULT ns_ix;

    CB_GetIndexRec(hwnd, IDL_NOISE_SHAPING, &ns_ix);
    srenders_get_vcfg(&sr_vcfg);
    sr_vcfg.nshape_type = ns_ix;
    srenders_set_vcfg(&sr_vcfg);
   }
   break;

  // player (system) setup
  case IDB_SYSETUP:
   if(plugin_systetup_dialog(hwnd, &the.cfg, agc))
   {
    // some immediately applies:
    mod_context_change_all_hilberts_config(&(the.cfg.iir_comp_config));
   }
   break;

  // toggle long/short number
  case IDC_LONG_NO:
   the.cfg.show_long_numbers = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_LONG_NO));
   updtxt_gain(hwnd, agc -> lCur -> l_gain, agc -> lCur -> r_gain);
   // mode specific update
   switch(agc -> lCur -> mode)
   {
    case MODE_SHIFT:
     updtxt_fshift(hwnd,
        agc -> lCur -> dsp.mk_shift.le.fr_shift, agc -> lCur -> dsp.mk_shift.ri.fr_shift);
     break;

    case MODE_PM:
     updtxt_pmfreq(hwnd,
        agc -> lCur -> dsp.mk_pm.le.freq, agc -> lCur -> dsp.mk_pm.ri.freq);
     updtxt_pmphase(hwnd,
        agc -> lCur -> dsp.mk_pm.le.phase, agc -> lCur -> dsp.mk_pm.ri.phase);
     updtxt_pmlevel(hwnd,
        agc -> lCur -> dsp.mk_pm.le.level, agc -> lCur -> dsp.mk_pm.ri.level);
     updtxt_pmangle(hwnd,
        agc -> lCur -> dsp.mk_pm.le.angle, agc -> lCur -> dsp.mk_pm.ri.angle);
     break;

    default:
     break;
   }
   break;

  // toggle 24/16 bits
  case IDC_ADV24BITS:
   the.cfg.need24bits = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_ADV24BITS));
   break;

  // toggle "ALT+3"
  case IDC_SHOWPLAY:
   setShowPlay(!!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHOWPLAY)));
   break;

  // -- all about FP exceptions::
  // toggle FP exceptions counters
  case IDC_SEXCP:
   ShowHideExceptions(hwnd, !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SEXCP)), agc);
   break;

  // partial resets of exceptions counters:
  case IDB_LH_RES:              // left - Hilbert
   fecs_resets(TRUE, FALSE, FALSE, FALSE);
   ShowFPExceptions(hwnd, TRUE, TRUE, agc);
   break;

  case IDB_LS_RES:              // left - render
   fecs_resets(FALSE, FALSE, TRUE, FALSE);
   ShowFPExceptions(hwnd, TRUE, TRUE, agc);
   break;

  case IDB_RH_RES:              // right - Hilbert
   fecs_resets(FALSE, TRUE, FALSE, FALSE);
   ShowFPExceptions(hwnd, TRUE, TRUE, agc);
   break;

  case IDB_RS_RES:              // right - render
   fecs_resets(FALSE, FALSE, FALSE, TRUE);
   ShowFPExceptions(hwnd, TRUE, TRUE, agc);
   break;

  // common reset of exceptions counters
  case IDB_FPALL_RES:
   fecs_resets(TRUE, TRUE, TRUE, TRUE);
   ShowFPExceptions(hwnd, TRUE, TRUE, agc);
   break;

  // termination
  case IDCANCEL:                // exit dialog -- free all
  case IDOK:                    // -"-
   // complete:
   if(!agc -> timer_id)
   {
    KillTimer(hwnd, agc -> timer_id);
    agc -> timer_id = 0;
   }
   EndDialog(hwnd, 0);
   break;
 }
}

#undef HANDLE_OUTPUT
#undef HANDLE_INPUT

/* WM_HSCROLL handling (track bars)
*/
static UINT Setup_Dlg_OnHscroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
 AMOD_GUI_CONTEXT *agc = (AMOD_GUI_CONTEXT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

 if(SB_THUMBTRACK == code)              // not handling
  return 1;

 // -- gain
 if(hdlbar_gain(hwnd, hwndCtl, agc))
  return 0;

 // -- frequency shift
 if(hdlbar_fshift(hwnd, hwndCtl, agc))
  return 0;

 // -- PM - frequency
 if(hdlbar_pmfreq(hwnd, hwndCtl, agc))
  return 0;

 // -- PM - phase (int)
 if(hdlbar_pmphase(hwnd, hwndCtl, agc))
  return 0;

 // -- PM - level
 if(hdlbar_pmlevel(hwnd, hwndCtl, agc))
  return 0;

 // -- PM - angle (ext)
 if(hdlbar_pmangle(hwnd, hwndCtl, agc))
  return 0;

 // not handled
 return 1;
}

/* the timer handler
*/
static void Setup_Dlg_OnTimer(HWND hwnd, UINT id)
{
 AMOD_GUI_CONTEXT *agc = (AMOD_GUI_CONTEXT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

 // here we check (and, if need, update GUI state) if the module state
 // was changed asynchronously
 // clipping statistics:
 ShowClipsPeaks(hwnd, FALSE, FALSE, agc);
 // frame counters:
 ShowFrameCounters(hwnd, FALSE, FALSE, FALSE, agc);
 // de-subnorn counters:
 ShowDesubnCounters(hwnd, FALSE, FALSE, FALSE, agc);
 // FP exception statistics:
 ShowFPExceptions(hwnd, the.cfg.is_fp_check, FALSE, agc);
}

/* the main dialog function
*/
static BOOL CALLBACK Setup_DlgProc(HWND hDlg, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
 BOOL fProcessed = TRUE;        // FALSE for WM_INITDIALOG if call SetFocus

 switch(uMsg)                   // message handling
 {
  HANDLE_MSG(hDlg, WM_INITDIALOG,       Setup_Dlg_OnInitDialog);
  HANDLE_MSG(hDlg, WM_COMMAND,          Setup_Dlg_OnCommand);
  HANDLE_MSG(hDlg, WM_HSCROLL,          Setup_Dlg_OnHscroll);
  HANDLE_MSG(hDlg, WM_TIMER,            Setup_Dlg_OnTimer);
  default:
   fProcessed = FALSE;
   break;
 }
 return fProcessed;
}

/* the front-end for advanced modulator GUI setup dialog
*/
void amgui_setup_dialog(HINSTANCE hi, HWND hwndParent)
{
 // nothing but about a static here:
 static int newnode_def_id = IDC_TYPE_SHIFT; // the default ID of new DSP node
 AMOD_GUI_CONTEXT aguic;

 memset(&aguic, 0, sizeof(aguic));
 aguic.lHead = NULL;
 aguic.lCur = NULL;
 aguic.loc_hi = hi;
 aguic.newnode_def_id = newnode_def_id;
 aguic.timer_id = 0;
 aguic.snfr_play = aguic.snfr_trans = 0ULL;
 aguic.snrj_play = aguic.snfr_trans = 0ULL;
 aguic.sl_cnt    = aguic.sr_cnt     = 0;
 aguic.sl_peak   = aguic.sr_peak    = 0.0;                  // zero values on form

 // aguic.fec_hilb_left, .fec_hilb_right, .fec_sr_left, .fec_sr_right are already zeroed here!!

 DialogBoxParam(hi, MAKEINTRESOURCE(IDD_ADV_SETUP), hwndParent, Setup_DlgProc, (LPARAM)&aguic);

 newnode_def_id = aguic.newnode_def_id;
}

/*
 * The "Add DSP node" dialog
 * --- ---- --- ----- ------
 */
// internal context of add DSP-node dialog
typedef struct tagADDDSP_CONTEXT
{
 NODE_DSP *node;                        // created node
 int newnode_def_id;                    // the default ID of new DSP node
} ADDDSP_CONTEXT;

// Message Handlers
// ------- --------
/* dialog initialization
*/
static BOOL AddDsp_Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
 ADDDSP_CONTEXT *adcn = (ADDDSP_CONTEXT *)lParam;

 SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)adcn);

 // Initial state
 CheckRadioButton(hwnd, IDC_TYPE_SHIFT, IDC_TYPE_MIX, adcn -> newnode_def_id);
 return TRUE;
}

/* command handling
*/
static void AddDsp_Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
 ADDDSP_CONTEXT *adcn = (ADDDSP_CONTEXT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
 int i;
 TCHAR nm[MAX_DSP_NAME + 2];

 static const struct mode_id    // ID - .mode
 {
  int chk_id;                   // Checked ID
  int mode;                     // Corresponding mode
 } md_ids[] =
 {
  { IDC_TYPE_SHIFT, MODE_SHIFT  },
  { IDC_TYPE_PM,    MODE_PM     },
  { IDC_TYPE_MIX,   MODE_MIX    },
  { 0, -1 }
 };

 switch(id)
 {
  case IDCANCEL:                // refuse
   EndDialog(hwnd, 0);
   break;
  case IDOK:                    // create
   for(i = 0; md_ids[i].chk_id != 0 && md_ids[i].mode >= 0; ++i)
   {
    if(BST_CHECKED == IsDlgButtonChecked(hwnd, md_ids[i].chk_id))
    {
     TXT_GetTxt(hwnd, IDC_NAME, nm, MAX_DSP_NAME);
     adcn -> newnode_def_id = md_ids[i].chk_id;
     adcn -> node = amod_add_lastdsp(nm, md_ids[i].mode);
     break;
    }
   }
   EndDialog(hwnd, 0);
   break;
 }
}

/* the main dialog function
*/
static BOOL CALLBACK AddDsp_DlgProc(HWND hDlg, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
 BOOL fProcessed = TRUE;        // FALSE for WM_INITDIALOG if call SetFocus

 switch(uMsg)                   // message handling
 {
  HANDLE_MSG(hDlg, WM_INITDIALOG,       AddDsp_Dlg_OnInitDialog);
  HANDLE_MSG(hDlg, WM_COMMAND,          AddDsp_Dlg_OnCommand);
  default:
   fProcessed = FALSE;
   break;
 }
 return fProcessed;
}

/* the front-end for Add DSP Node dialog
*/
static NODE_DSP *add_dsp_node_dialog(HWND hwndParent, AMOD_GUI_CONTEXT *agc)
{
 ADDDSP_CONTEXT adnc;

 adnc.node = INVALID_NODE_DSP;
 adnc.newnode_def_id = agc -> newnode_def_id;

 DialogBoxParam(agc -> loc_hi, MAKEINTRESOURCE(IDD_NEW_DSP),
        hwndParent, AddDsp_DlgProc, (LPARAM)&adnc);

 if(INVALID_NODE_DSP != adnc.node)
 {
  // not a Cancel
  agc -> newnode_def_id = adnc.newnode_def_id;
 }
 return adnc.node;                      // can be INVALID_NODE_DSP, NULL or real node ptr
}

/* The "Edit (double) value" dialog
 * --- --------------------- ------
 */
// internal context of Edit Value dialog
typedef struct tagEDIT_VALUE
{                                       // the list of parameters of edit_value_dialog()
 HWND hwndParent;                       // setup window
 int id_par_slider;                     // the slider to overlap
 const TCHAR *title;                    // parameter description
 double *val;                           // ptr to returned value
} EDIT_VALUE;

// Message Handlers
// ------- --------
/* dialog initialization
*/
static BOOL EditValue_Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
 EDIT_VALUE *ev = (EDIT_VALUE *)lParam;
 RECT rcp;

 SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ev);

 if(!GetWindowRect(GetDlgItem(ev -> hwndParent, ev -> id_par_slider), &rcp))
 {
  memset(&rcp, 0, sizeof(rcp));
 }

 SetWindowPos(hwnd, HWND_TOP,
        rcp.left + 5, rcp.top + 5, 0, 0,
        SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

 TXT_PrintTxt(hwnd, IDS_GETVALUE, _T("%s"), ev -> title);
 TXT_PrintTxt(hwnd, IDC_GETVALUE, _T("%.16G"), *(ev -> val));
 SetFocus(GetDlgItem(hwnd, IDC_GETVALUE));
 return TRUE;
}

/* command handling
*/
static void EditValue_Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
 EDIT_VALUE *ev = (EDIT_VALUE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

 switch(id)
 {
  // -- dialog general
  case IDCANCEL:                        // ESC - refuse
   EndDialog(hwnd, (INT_PTR)0);
   break;

  case IDOK:                            // ENTER - accept
   *(ev->val) = TXT_GetDbl(hwnd, IDC_GETVALUE, *(ev->val), 1.0, -1.0 /* w/o checks */, NULL);
   EndDialog(hwnd, (INT_PTR)1);
   break;
 }
}

/* the main dialog function
*/
static BOOL CALLBACK EditValue_DlgProc(HWND hDlg, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
 BOOL fProcessed = TRUE;                // FALSE for WM_INITDIALOG if call SetFocus

 switch(uMsg)                           // message handling
 {
  HANDLE_MSG(hDlg, WM_INITDIALOG,       EditValue_Dlg_OnInitDialog);
  HANDLE_MSG(hDlg, WM_COMMAND,          EditValue_Dlg_OnCommand);
  default:
   fProcessed = FALSE;
   break;
 }
 return fProcessed;
}

/* the front-end edit value dialog, TRUE == OK
*/
static BOOL edit_value_dialog(HWND hwndParent,  int id_par_slider,
        const TCHAR *title, double *val, AMOD_GUI_CONTEXT *agc)
{
 EDIT_VALUE ev;
 INT_PTR res;

 ev.hwndParent = hwndParent;
 ev.id_par_slider = id_par_slider;
 ev.title = title;
 ev.val = val;

 res = DialogBoxParam(agc -> loc_hi, MAKEINTRESOURCE(IDD_GETVALUE),
        hwndParent, EditValue_DlgProc, (LPARAM)&ev);

 return (res && (res != -1));
}

/* The "Plugin setup" i.e. "Bugs management" dialog
 * --- ------- ------ ---- ----------------- ------
 */
// internal context of payer (system) setup
typedef struct tagSYSETUP_CONTEXT
{
 BOOL is_wav_support;                           // true, if the module support WAV
 BOOL is_rwave_support;                         // true, if the module support RWAVE ext. for WAV
 unsigned infobox_parenting;                    // value INFOBOX_xxx - behaviour of "Alt-3" window
 BOOL enable_unload_cleanup;                    // enable cleanup on unload plugin
 unsigned play_sleep;                           // sleep while playback, ms
 BOOL disable_play_sleep;                       // disable sleep on plyback
 IIR_COMP_CONFIG icc;                           // IIR comp. rules
 unsigned sec_align;                            // time in seconds to align file length
 unsigned fade_in;                              // track fade in, ms
 unsigned fade_out;                             // track fade out, ms
 BOOL is_frmod_scaled;                          // true, if unsigned scaled modulation freqencies in use
} SYSETUP_CONTEXT;

// Message Handlers
// ------- --------
/* dialog initialization
*/
static BOOL SySetup_Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
 static const int parent_ids[] =                // indexes are INFOBOX_xxx
 {
  IDC_PAR_NOPARENT, IDC_PAR_FILELIST, IDC_PAR_MAINWND
 };

 SYSETUP_CONTEXT *ssc = (SYSETUP_CONTEXT *)lParam;

 SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ssc);

 // Initial state
 CheckDlgButton(hwnd, IDC_WAVEXT, ssc -> is_wav_support);
 CheckDlgButton(hwnd, IDC_RWAVEEXT, ssc -> is_rwave_support);

 CheckRadioButton(hwnd, parent_ids[INFOBOX_NOPARENT], parent_ids[INFOBOX_MAINPARENT],
        parent_ids[ssc -> infobox_parenting]);

 CheckDlgButton(hwnd, IDC_LAST_CHANCE, ssc -> enable_unload_cleanup);

 TXT_PrintTxt(hwnd, IDC_SLEEPTIME, _T("%u"), ssc -> play_sleep);
 EnDis(hwnd, IDC_SLEEPTIME, !ssc -> disable_play_sleep);
 EnDis(hwnd, IDB_SLEEPTIME, !ssc -> disable_play_sleep);
 CheckDlgButton(hwnd, IDC_NOSLEEP, ssc -> disable_play_sleep);

 TXT_PrintTxt(hwnd, IDC_SUB_THR, _T("%.14G"), ssc -> icc.subnorm_thr);
 CheckDlgButton(hwnd, IDC_RJ_SUB_THR, ssc -> icc.is_subnorm_reject);
 CheckDlgButton(hwnd, IDC_KAHAN_SUM,  ssc -> icc.is_kahan);

 TXT_PrintTxt(hwnd, IDC_ALIGN,    _T("%u"), ssc -> sec_align);
 TXT_PrintTxt(hwnd, IDC_FADE_IN,  _T("%u"), ssc -> fade_in);
 TXT_PrintTxt(hwnd, IDC_FADE_OUT, _T("%u"), ssc -> fade_out);

 CheckRadioButton(hwnd, IDC_MODFR_SCALED, IDC_MODFR_NATIVE,
        ssc -> is_frmod_scaled? IDC_MODFR_SCALED : IDC_MODFR_NATIVE);

 return TRUE;
}

/* command handling
*/
static void SySetup_Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
 SYSETUP_CONTEXT *ssc = (SYSETUP_CONTEXT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

 switch(id)
 {
  // -- setup extensions
  case IDC_WAVEXT:                      // WAV support
   ssc -> is_wav_support = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_WAVEXT));
   break;

  case IDC_RWAVEEXT:                    // RWAVE support
   ssc -> is_rwave_support = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RWAVEEXT));
   break;

  // -- setup parenting
  case IDC_PAR_NOPARENT:                // no parent to InfoBox
   ssc -> infobox_parenting = INFOBOX_NOPARENT;
   break;

  case IDC_PAR_FILELIST:                // no parent to InfoBox
   ssc -> infobox_parenting = INFOBOX_LISTPARENT;
   break;

  case IDC_PAR_MAINWND:                 // no parent to InfoBox
   ssc -> infobox_parenting = INFOBOX_MAINPARENT;
   break;

  // -- setup saving config mode
  case IDC_LAST_CHANCE:                 // saving from DllMain()
   ssc -> enable_unload_cleanup = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_LAST_CHANCE));
   break;

  // -- setup sleep@playback
  case IDB_SLEEPTIME:                   // accept sleeptime
   ssc -> play_sleep = TXT_GetUlng(hwnd, IDC_SLEEPTIME, DEF_PLAY_SLEEP, 0, MAX_PLAY_SLEEP);
   break;

  case IDC_NOSLEEP:                     // set disable sleeping
   ssc -> disable_play_sleep = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_NOSLEEP));
   TXT_PrintTxt(hwnd, IDC_SLEEPTIME, _T("%u"), ssc -> play_sleep);
   EnDis(hwnd, IDC_SLEEPTIME, !ssc -> disable_play_sleep);
   EnDis(hwnd, IDB_SLEEPTIME, !ssc -> disable_play_sleep);
   break;

  // setup IIR computations rules
  case IDB_SUB_THR:                     // accept highest value should be zeroed
   ssc -> icc.subnorm_thr = TXT_GetDbl(hwnd
        , IDC_SUB_THR
        , ssc -> icc.subnorm_thr
        , SBN_THR_MIN
        , SBN_THR_MAX
        , NULL);
   break;

  case IDC_RJ_SUB_THR:                  // toggle subnorm protection
   ssc -> icc.is_subnorm_reject = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RJ_SUB_THR));
   break;

  case IDC_KAHAN_SUM:                   // toggle Kahan summation
   ssc -> icc.is_kahan = !!(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_KAHAN_SUM));
   break;

  // -- track framing setup
  case IDB_TRACCEPT:                    // set align time, fade in and fade out
   ssc -> sec_align = TXT_GetUlng(hwnd, IDC_ALIGN,    0, 0, MAX_ALIGN_SEC);
   ssc -> fade_in   = TXT_GetUlng(hwnd, IDC_FADE_IN,  0, 0, MAX_FADE_INOUT);
   ssc -> fade_out  = TXT_GetUlng(hwnd, IDC_FADE_OUT, 0, 0, MAX_FADE_INOUT);
   break;

  // -- setup modulation freqency scaling
  case IDC_MODFR_SCALED:                // switch to scaled mode
   ssc -> is_frmod_scaled = TRUE;
   break;

  case IDC_MODFR_NATIVE:                // switch to non-scaled mode
   ssc -> is_frmod_scaled = FALSE;
   break;

  // -- dialog general
  case IDCANCEL:                        // refuse
   EndDialog(hwnd, (INT_PTR)0);
   break;

  case IDOK:                            // accept
   // last chance to update numerical parameters
   ssc -> play_sleep = TXT_GetUlng(hwnd, IDC_SLEEPTIME, DEF_PLAY_SLEEP, 0, MAX_PLAY_SLEEP);

   ssc -> icc.subnorm_thr = TXT_GetDbl(hwnd
        , IDC_SUB_THR
        , ssc -> icc.subnorm_thr
        , SBN_THR_MIN
        , SBN_THR_MAX
        , NULL);

   ssc -> sec_align = TXT_GetUlng(hwnd, IDC_ALIGN,    0, 0, MAX_ALIGN_SEC);
   ssc -> fade_in   = TXT_GetUlng(hwnd, IDC_FADE_IN,  0, 0, MAX_FADE_INOUT);
   ssc -> fade_out  = TXT_GetUlng(hwnd, IDC_FADE_OUT, 0, 0, MAX_FADE_INOUT);

   EndDialog(hwnd, (INT_PTR)ssc);
   break;
 }
}

/* the main dialog function
*/
static BOOL CALLBACK SySetup_DlgProc(HWND hDlg, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
 BOOL fProcessed = TRUE;                // FALSE for WM_INITDIALOG if call SetFocus

 switch(uMsg)                           // message handling
 {
  HANDLE_MSG(hDlg, WM_INITDIALOG,       SySetup_Dlg_OnInitDialog);
  HANDLE_MSG(hDlg, WM_COMMAND,          SySetup_Dlg_OnCommand);
  default:
   fProcessed = FALSE;
   break;
 }
 return fProcessed;
}

/* the front-end for plugin setup gialog, TRUE==OK, FALSE==Cancel
*/
static BOOL plugin_systetup_dialog(HWND hwndParent,
        IN_CWAVE_CFG *ic_conf, AMOD_GUI_CONTEXT *agc)
{
 SYSETUP_CONTEXT ssc;
 INT_PTR res;

 ssc.is_wav_support         = ic_conf -> is_wav_support;
 ssc.is_rwave_support       = ic_conf -> is_rwave_support;
 ssc.infobox_parenting      = ic_conf -> infobox_parenting;
 ssc.enable_unload_cleanup  = ic_conf -> enable_unload_cleanup;
 ssc.play_sleep             = ic_conf -> play_sleep;
 ssc.disable_play_sleep     = ic_conf -> disable_play_sleep;

 memcpy(&(ssc.icc), &(ic_conf -> iir_comp_config), sizeof(IIR_COMP_CONFIG));

 ssc.sec_align              = ic_conf -> sec_align;
 ssc.fade_in                = ic_conf -> fade_in;
 ssc.fade_out               = ic_conf -> fade_out;
 ssc.is_frmod_scaled        = ic_conf -> is_frmod_scaled;

 res = DialogBoxParam(agc -> loc_hi, MAKEINTRESOURCE(IDD_SYSETUP),
        hwndParent, SySetup_DlgProc, (LPARAM)&ssc);

 if(res && res != -1)
 {
  if(ssc.is_wav_support != ic_conf -> is_wav_support
        || ssc.is_rwave_support != ic_conf -> is_rwave_support)
  {
   if(IDOK == MessageBox(hwndParent,
        _T("You make changes in supported file types\n")
        _T("You must to restart player and, probably,\n")
        _T("to make an additional setup in player configuration"),
        _T("Need to restart"),
        MB_OKCANCEL | MB_ICONWARNING))
   {
    ic_conf -> is_wav_support = ssc.is_wav_support;
    ic_conf -> is_rwave_support = ssc.is_rwave_support;
   }
  }

  ic_conf -> infobox_parenting = ssc.infobox_parenting;
  ic_conf -> enable_unload_cleanup = ssc.enable_unload_cleanup;

  if(ssc.disable_play_sleep || !ssc.play_sleep)
  {
   if(IDOK == MessageBox(hwndParent,
        _T("You set zero sleep time while playback.\n")
        _T("It's useful for transcode files with XMPlay;\n")
        _T("but turn plugin to 100% CPU load while playback"),
        _T("Set 100% CPU load at playback"),
        MB_OKCANCEL | MB_ICONWARNING))
   {
    ic_conf -> play_sleep = ssc.play_sleep;
    ic_conf -> disable_play_sleep = ssc.disable_play_sleep;
   }
  }

  // unchecked cases
  if(ssc.play_sleep)
   ic_conf -> play_sleep = ssc.play_sleep;
  if(!ssc.disable_play_sleep)
   ic_conf -> disable_play_sleep = ssc.disable_play_sleep;

  memcpy(&(ic_conf -> iir_comp_config), &(ssc.icc), sizeof(IIR_COMP_CONFIG));

  ic_conf -> sec_align = ssc.sec_align;
  ic_conf -> fade_in   = ssc.fade_in;
  ic_conf -> fade_out  = ssc.fade_out;

  // milihertz or hertz
  if(ic_conf -> is_frmod_scaled != ssc.is_frmod_scaled)
  {
   MessageBox(hwndParent,
        _T("You had changed the rules of phase computation.\n")
        _T("Restart player to apply the changes."),
        _T("Change Phase Rules"),
        MB_OK | MB_ICONWARNING);
   ic_conf -> is_frmod_scaled = ssc.is_frmod_scaled;
  }

  return TRUE;
 }                      // if(OK@Dlg)

 return FALSE;          // if(Cancel@Dlg)
}

/* the end...
*/

