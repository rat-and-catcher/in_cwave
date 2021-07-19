/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      adv_modulator.c -- advanced analitic signal modulator
 *      (there is no any "simple" renders / mods from in_cwave V1.5.0)
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

#include "in_cwave.h"

// get scaled frequency as scaled unsigned
#define UGET_SCALED_FR(df)          ((unsigned)((df) * ((double)HZ_SCALE) + 0.5))
// get scaled frequency as scaled double
#define DGET_SCALED_FR(df)          ((double)UGET_SCALED_FR(df))
// get scaled frequency as unscaled double
#define DGET_USCALED_FR(df)         (DGET_SCALED_FR(df) / ((double)HZ_SCALE))

/*
 * global statics
 * ------ -------
 */
typedef struct tagADVANCED_MODULATOR
{
 NODE_DSP *head;                                // head/base .master of DSP list
 NODE_DSP *tail;                                // pointer of the tail of DSP list
 CRITICAL_SECTION cs_dsp_list;                  // DSP-list protector
 unsigned l_clips, r_clips;                     // clips per channel
 double l_peak, r_peak;                         // peak values per chaannel, dB
 BOOL is_bypass_list;                           // DSP list bypass flag
 BOOL is_frmod_scaled;                          // copyed from config at one-time init
} ADVANCED_MODULATOR;

static ADVANCED_MODULATOR am;                   // one and only one copy

static const TCHAR *dsp_names[] =               // Names (or prefixes) of DSP-node
{
 _T("Master"),                                  // MODE_MASTER (0)
 _T("Shift-"),                                  // MODE_SHIFT (1)
 _T("PM-"),                                     // MODE_PM (2)
 _T("MIX-")                                     // MODE_MIX (3)
};

/* Helpers
 * -------
 */
/* helper to create new node
*/
static NODE_DSP *create_node_dsp(const TCHAR *name, int mode, int force_master)
{
 NODE_DSP *temp = (NODE_DSP *)cmalloc(sizeof(NODE_DSP));
 int i;

 memset(temp, 0, sizeof(NODE_DSP));

 temp -> prev = temp -> next = NULL;
 temp -> mode = mode;

 // from here we fill ALL the fields of new node with
 // adequate values; this don't mean, that the caller will not
 // redefine them to it's own defaults.
 temp -> l_gain = temp -> r_gain = DEF_GAIN_MOD;
 for(i = 0; i < N_INPUTS; ++i)
  temp -> inputs[i] = 0;

 temp -> xch_mode = XCH_DEF;
 temp -> l_iq_invert = temp -> r_iq_invert = 0;

 switch(mode)
 {
  default:
  case MODE_MASTER:                     // only if force_master != 0
   temp -> l_gain = temp -> r_gain = DEF_GAIN_MASTER;
   if(force_master)
   {
    temp -> dsp.mk_master.le.tout = temp -> dsp.mk_master.ri.tout = S_RE;
    temp -> inputs[0] = 1;              // master initially always has 'in' On
   }
   else
   {
    free(temp);
    return NULL;
   }
   break;

  case MODE_SHIFT:
   temp -> dsp.mk_shift.le.fr_shift =  DEF_FSHIFT;
   temp -> dsp.mk_shift.le.is_shift = 1;

   temp -> dsp.mk_shift.ri.fr_shift = -DEF_FSHIFT;
   temp -> dsp.mk_shift.ri.is_shift = 1;

   temp -> dsp.mk_shift.n_out = N_INPUTS - 1;
   temp -> dsp.mk_shift.lock_shift = 1;
   temp -> dsp.mk_shift.sign_lock_shift = 1;
   break;

  case MODE_PM:
   temp -> dsp.mk_pm.le.freq = DEF_PMFREQ;
   temp -> dsp.mk_pm.le.phase = 0.0;
   temp -> dsp.mk_pm.le.level = DEF_PMLEVEL;
   temp -> dsp.mk_pm.le.angle = 0.0;
   temp -> dsp.mk_pm.le.is_pm = 1;

   temp -> dsp.mk_pm.ri.freq = DEF_PMFREQ;
   temp -> dsp.mk_pm.ri.phase = DEF_PMPHASE;
   temp -> dsp.mk_pm.ri.level = DEF_PMLEVEL;
   temp -> dsp.mk_pm.ri.angle = DEF_PMANGLE;
   temp -> dsp.mk_pm.ri.is_pm = 1;

   temp -> dsp.mk_pm.n_out = N_INPUTS - 1;
   temp -> dsp.mk_pm.lock_freq = 1;
   temp -> dsp.mk_pm.lock_phase = 0;
   temp -> dsp.mk_pm.lock_level = 1;
   temp -> dsp.mk_pm.lock_angle = 0;
   break;

  case MODE_MIX:
   temp -> dsp.mk_mix.n_out = N_INPUTS - 1;
   break;
 }

 // the name
 _tcscpy(temp -> name, dsp_names[mode]);
 if(name)
  _tcscat(temp -> name, name);

 temp -> lock_gain = 1;

 // ready to include to list
 return temp;
}

/* remove (replace) node outplug -- thread unsafe
*/
static void replace_output_plug(NODE_DSP *ndEd, int n)  // n == -1 -> remove only
{
 int nrem = -1;

 switch(ndEd -> mode)
 {
  default:
  case MODE_MASTER:
   // just in case...
   break;

  case MODE_SHIFT:
   nrem = ndEd -> dsp.mk_shift.n_out;
   if(n >= 0)
    ndEd -> dsp.mk_shift.n_out = n;
   break;

  case MODE_PM:
   nrem = ndEd -> dsp.mk_pm.n_out;
   if(n >= 0)
    ndEd -> dsp.mk_pm.n_out = n;
   break;

  case MODE_MIX:
   nrem = ndEd -> dsp.mk_mix.n_out;
   if(n >= 0)
    ndEd -> dsp.mk_mix.n_out = n;
   break;
 }

 // clear unused inout
 if(nrem >= 0)
  mod_context_clear_all_inouts(nrem);
}

/* Front-end functions
 * --------- ---------
 */
/* one-time initialization of advanced modulator
*/
int amod_init(NODE_DSP *cfg_list, BOOL is_frmod_scaled)         // !0, if list accepted
{
 int res = 0;
 int was_master = 0;

 memset(&am, 0, sizeof(am));
 am.head = NULL;                                                // paranoja ** 2

 if(cfg_list)
 {
  NODE_DSP *temp;

  // Try to aacept external DSP-list. We assume, that all of the fields of a single node are
  // correct; we just check, that the master is one and only one and she live at head of the
  // list position. If cfg_list not accepted, _we shall free() it_.
  if(cfg_list -> mode == MODE_MASTER)
  {
   temp = am.head = cfg_list;

   // no chance to change the master's name from config ;)
   _tcscpy(cfg_list -> name, dsp_names[MODE_MASTER]);
   do
   {
    // make few semantic checks
    if(temp -> lock_gain)
    {
     temp -> r_gain = temp -> l_gain;
     temp -> r_iq_invert = temp -> l_iq_invert;
    }
    switch(temp -> mode)
    {
     case MODE_MASTER:
      // to be sure that master is one and only one, and stay first in the list
      if(was_master)
       am.head = NULL;
      else
       was_master = 1;
      break;

     case MODE_SHIFT:
      if(temp -> dsp.mk_shift.lock_shift)
      {
       temp -> dsp.mk_shift.ri.fr_shift =
        temp -> dsp.mk_shift.sign_lock_shift?
                -temp -> dsp.mk_shift.le.fr_shift
                :
                temp -> dsp.mk_shift.le.fr_shift;
       temp -> dsp.mk_shift.ri.is_shift = temp -> dsp.mk_shift.le.is_shift;
      }
      break;

     case MODE_PM:
      if(temp -> dsp.mk_pm.lock_freq)
      {
       temp -> dsp.mk_pm.ri.freq = temp -> dsp.mk_pm.le.freq;
       temp -> dsp.mk_pm.ri.is_pm = temp -> dsp.mk_pm.le.is_pm;
      }
      if(temp -> dsp.mk_pm.lock_phase)
      {
       temp -> dsp.mk_pm.ri.phase = temp -> dsp.mk_pm.le.phase;
      }
      if(temp -> dsp.mk_pm.lock_level)
      {
       temp -> dsp.mk_pm.ri.level = temp -> dsp.mk_pm.le.level;
      }
      if(temp -> dsp.mk_pm.lock_angle)
      {
       temp -> dsp.mk_pm.ri.angle = temp -> dsp.mk_pm.le.angle;
      }
      break;

     case MODE_MIX:
      break;

     default:
      am.head = NULL;
      break;
    }

    am.tail = temp;
    temp = temp -> next;
   }
   while(temp && am.head);
  }

  if(am.head)
  {
   res = 1;
  }
  else
  {
   while(cfg_list)
   {
    temp = cfg_list;
    cfg_list = cfg_list -> next;
    free(temp);
   }
  }
 }

 if(!am.head)
 {
  // create the master node -- one and only one
  am.head = create_node_dsp(NULL, MODE_MASTER, 1 /* enable to create master */);
  am.tail = am.head;
 }

 // the rest of modulator's object
 InitializeCriticalSection(&am.cs_dsp_list);
 am.l_clips = am.r_clips = 0;
 am.l_peak = am.r_peak = SR_ZERO_SIGNAL_DB;
 am.is_bypass_list = FALSE;
 am.is_frmod_scaled = is_frmod_scaled;                          // need restart to change

 return res;
}

/* cleanup advanced modulator (+, probably transfer DSP-list)
*/
NODE_DSP *amod_cleanup(int is_transfer)
{
 NODE_DSP *res = NULL;

 if(is_transfer)
 {
  res = am.head;
 }
 else
 {
  if(am.head)
  {
   amod_del_dsplist();
   free(am.head);
  }
 }
 // ok, looks mad:
 am.head = am.tail = NULL;
 DeleteCriticalSection(&am.cs_dsp_list);

 return res;                                    // may be NULL in lots of cases
}

/* delete the whole DSP list
*/
void amod_del_dsplist(void)
{
 EnterCriticalSection(&am.cs_dsp_list);
 while(am.tail -> prev)
 {
  am.tail = am.tail -> prev;
  replace_output_plug(am.tail -> next, -1);     // clear unused plug
  free(am.tail -> next);
  am.tail -> next = NULL;
 }
 LeaveCriticalSection(&am.cs_dsp_list);
}

/* delete the last DSP list element
*/
void amod_del_lastdsp(void)
{
 EnterCriticalSection(&am.cs_dsp_list);
 if(am.tail -> prev)
 {
  am.tail = am.tail -> prev;
  replace_output_plug(am.tail -> next, -1);     // clear unused plug
  free(am.tail -> next);
  am.tail -> next = NULL;
 }
 LeaveCriticalSection(&am.cs_dsp_list);
}

/* add the last element to the DSP list
*/
NODE_DSP *amod_add_lastdsp(const TCHAR *name, int mode)
{
 NODE_DSP *temp = create_node_dsp(name, mode, 0 /* can't create master from here */);

 if(NULL == temp)
  return temp;

 // ok, include to the list
 EnterCriticalSection(&am.cs_dsp_list);
 temp -> prev = am.tail;
 am.tail -> next = temp;
 am.tail = temp;
 LeaveCriticalSection(&am.cs_dsp_list);

 return am.tail;
}

/* get the state of bypass list flag
*/
BOOL amod_get_bypass_list_flag(void)
{
 return am.is_bypass_list;
}

/* set the state of bypass list flag
*/
void amod_set_bypass_list_flag(BOOL bypass)
{
 am.is_bypass_list = bypass;
}

/* get DSP list head for keep-the-structure operation
*/
NODE_DSP *amod_get_headdsp(void)
{
 return am.head;
}

/* set DSP node outplug -- thread safe
*/
void amod_set_output_plug(NODE_DSP *ndEd, int n)        // n == -1 -> remove only
{
 EnterCriticalSection(&am.cs_dsp_list);
 replace_output_plug(ndEd, n);
 LeaveCriticalSection(&am.cs_dsp_list);
}

/* get channel's clips counters and peak values
*/
void amod_get_clips_peaks
    ( unsigned *lc
    , unsigned *rc
    , double *lpv
    , double *rpv
    , BOOL isReset
    )
{
 if(isReset)
 {
  am.l_clips = am.r_clips = 0;
  am.l_peak = am.r_peak = SR_ZERO_SIGNAL_DB;
 }

 // asynchronious changes of X_clips uncritical
 *lc = am.l_clips;
 *rc = am.r_clips;
 // asynchronious changes of X_peak uncritical
 adbl_write(lpv, am.l_peak);
 adbl_write(rpv, am.r_peak);
}

/* convert a frequency to it's "true" value
*/
double amod_true_freq(double raw_freq)
{
 return am.is_frmod_scaled?
    (raw_freq < 0? -DGET_USCALED_FR(-raw_freq) : DGET_USCALED_FR(raw_freq))
    :
    raw_freq;
}

/* DSP helpers
 * --- -------
 */
/* NOTE:: We trying to use volatile variables only once and don't
 * include them to complicated expressions...
 */
/* make one channel for CMAKE_MASTER
*/
static __inline double dsp_master(CMAKE_MASTER *master, CCOMPLEX *input)
{
 switch(master -> tout)
 {
  case S_RE:
   return input -> re;
   break;

  case S_IM:
   return input -> im;
   break;
 }
 // default -- some sort of 'assert'
 return 0.0;
}

/* make copy of complex (CCOMPLEX *)
*/
static __inline void dsp_copy(CCOMPLEX *output, CCOMPLEX *input)
{
 output -> re = input -> re;
 output -> im = input -> im;
}

/* make one channel for CMAKE_SHIFT
*/
static __inline void dsp_shift(CMAKE_SHIFT *shift, CCOMPLEX *output,
        CCOMPLEX *input, double norm_omega)
{
 if(shift -> is_shift)
 {
  double sh_freq, phase, cos_v, sin_v;
  BOOL sh_sign = FALSE;

  adbl_read(&sh_freq, &(shift -> fr_shift));
  if(sh_freq < 0.0)
  {
   sh_freq = -sh_freq;
   sh_sign = TRUE;
  }
  if(am.is_frmod_scaled)
  {
   sh_freq = DGET_SCALED_FR(sh_freq);
  }
  phase = fmod(norm_omega * sh_freq, 2.0 * PI); // fmod() don't need
  cos_v = cos(phase);
  sin_v = sin(phase);
  if(sh_sign)
   sin_v = -sin_v;

  output -> re = input -> re * cos_v - input -> im * sin_v;
  output -> im = input -> re * sin_v + input -> im * cos_v;
 }
 else
 {
  dsp_copy(output, input);
 }
}

/* make one channel for CMAKE_PM
*/
static __inline void dsp_pm(CMAKE_PM *pm, CCOMPLEX *output,
        CCOMPLEX *input, double norm_omega)
{
 if(pm -> is_pm)
 {
  double freq, fphase, flevel, fangle, phase;

  adbl_read(&freq,    &(pm -> freq));
  adbl_read(&fphase,  &(pm -> phase));
  adbl_read(&flevel,  &(pm -> level));
  adbl_read(&fangle,  &(pm -> angle));
  if(am.is_frmod_scaled)
  {
   freq = DGET_SCALED_FR(freq);
  }
  phase = fmod(norm_omega * freq, 2.0 * PI);    // fmod() don't need
  {
   double psi = flevel * PI * (sin(phase + fphase * PI) + fangle);
   double cos_v = cos(psi);
   double sin_v = sin(psi);
   output -> re = input -> re * cos_v - input -> im * sin_v;
   output -> im = input -> re * sin_v + input -> im * cos_v;
  }
 }
 else
 {
  dsp_copy(output, input);
 }
}

/* render ns <= NS_PERTIME samples into buf
*/
int amod_process_samples(char *buf, MOD_CONTEXT *mc)
{
// this function is only used by DecodeThread. (NO-NO-NO!! transcode too!! ;))
// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will be still work, but some of the visualization
// might not look as good as it could. Stick with NS_PERTIME (576) sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if
// necessary..

 double norm_omega;
 double lOut = 0.0, rOut = 0.0;         // to shut up code analiser
 NODE_DSP *cur;
 unsigned ix_mix;

 if(!xwave_read_samples(mc -> xr))
  return 0;

 while(mc -> xr -> unpacked < mc -> xr -> really_readed)
 {
  // In V02.00.00 this was:
  // norm_omega = (2.0 * PI) * (double)(mc -> n_frame) / mc -> xr -> sample_rate;
  // mc -> n_frame = (mc -> n_frame + 1) % mc -> xr -> sample_rate;
  // Its sound proudly, but worked correctly only for integer modulation freqs.

  mod_context_lock_framecnt(mc);
  if(am.is_frmod_scaled)
  {
   unsigned scale_sr = mc -> xr -> sample_rate * HZ_SCALE;

   norm_omega = (2.0 * PI) * ((double)(mc -> n_frame)) / ((double)scale_sr);
   mc -> n_frame = (mc -> n_frame + 1) % (uint64_t)scale_sr;
  }
  else
  {
   // directly and literally
   norm_omega = (2.0 * PI) * ((double)(mc -> n_frame)) / (double)(mc -> xr -> sample_rate);
   ++(mc -> n_frame);                               // ..unlimitly.. ..to hell..
  }
  mod_context_unlock_framecnt(mc);

  all_hilberts_lock();
  xwave_unpack_csample(
        &mc -> inout[0].le.re, &mc -> inout[0].le.im,
        &mc -> inout[0].ri.re, &mc -> inout[0].ri.im,
        mc, mc -> xr);
  all_hilberts_unlock();

  // loop by the DSP list from TAIL to HEAD
  EnterCriticalSection(&am.cs_dsp_list);
  for(cur = am.is_bypass_list? am.head : am.tail; cur; cur = cur -> prev)
  {
   LRCOMPLEX data, *pout;
   double lg, rg;
   double xt;

   // make mix
   if(am.is_bypass_list)
   {
    // take raw "in" only
    data.le.re = mc -> inout[0].le.re;
    data.le.im = mc -> inout[0].le.im;
    data.ri.re = mc -> inout[0].ri.re;
    data.ri.im = mc -> inout[0].ri.im;
   }
   else
   {
    // the true mix
    data.le.re = data.le.im = data.ri.re = data.ri.im = 0.0;
    for(ix_mix = 0; ix_mix < N_INPUTS; ++ix_mix)
    {
     if(cur -> inputs[ix_mix])
     {
      data.le.re += mc -> inout[ix_mix].le.re;
      data.le.im += mc -> inout[ix_mix].le.im;
      data.ri.re += mc -> inout[ix_mix].ri.re;
      data.ri.im += mc -> inout[ix_mix].ri.im;
     }
    }
   }

   // channels exchange
   switch(cur -> xch_mode)
   {
    case XCH_SWAP:                                      // swap left and right
     xt = data.le.re;
     data.le.re = data.ri.re;
     data.ri.re = xt;

     xt = data.le.im;
     data.le.im = data.ri.im;
     data.ri.im = xt;
     break;

    case XCH_LEFTONLY:                                  // both set to left
     data.ri.re = data.le.re;
     data.ri.im = data.le.im;
     break;

    case XCH_RIGHTONLY:                                 // both set to right
     data.le.re = data.ri.re;
     data.le.im = data.ri.im;
     break;

    case XCH_MIXLR:                                     // both set to (L + R) / 2
     data.le.re = data.ri.re = (data.le.re + data.ri.re) / 2.0;
     data.le.im = data.ri.im = (data.le.im + data.ri.im) / 2.0;
     break;

    case XCH_NORMAL:                                    // leave as is
    default:
     break;

   }

   // spectrum inversion (swap I<>Q)
   if(cur -> l_iq_invert)
   {
    xt = data.le.re;
    data.le.re = data.le.im;
    data.le.im = xt;
   }
   if(cur -> r_iq_invert)
   {
    xt = data.ri.re;
    data.ri.re = data.ri.im;
    data.ri.im = xt;
   }

   // adjust levels (make _after_ channels exchange)
   adbl_read(&lg, &(cur -> l_gain));
   adbl_read(&rg, &(cur -> r_gain));
   data.le.re *= lg;
   data.le.im *= lg;
   data.ri.re *= rg;
   data.ri.im *= rg;

   // exec DSP
   switch(cur -> mode)
   {
    case MODE_MASTER:
     lOut = dsp_master(&(cur -> dsp.mk_master.le), &data.le);
     rOut = dsp_master(&(cur -> dsp.mk_master.ri), &data.ri);
     // .. here we should be sure, that lOut/rOut is the final product of DSP ..
     break;

    case MODE_SHIFT:
     pout = &(mc -> inout[cur -> dsp.mk_shift.n_out]);
     dsp_shift(&(cur -> dsp.mk_shift.le), &(pout -> le), &data.le, norm_omega);
     dsp_shift(&(cur -> dsp.mk_shift.ri), &(pout -> ri), &data.ri, norm_omega);
     break;

    case MODE_PM:
     pout = &(mc -> inout[cur -> dsp.mk_pm.n_out]);
     dsp_pm(&(cur -> dsp.mk_pm.le), &(pout -> le), &data.le, norm_omega);
     dsp_pm(&(cur -> dsp.mk_pm.ri), &(pout -> ri), &data.ri, norm_omega);
     break;

    case MODE_MIX:
     pout = &(mc -> inout[cur -> dsp.mk_mix.n_out]);
     dsp_copy(&(pout -> le), &data.le);
     dsp_copy(&(pout -> ri), &data.ri);
     break;
   }                                            // switch .mode
  }                                             // END of DSP loop
  LeaveCriticalSection(&am.cs_dsp_list);

  // place into out buffer
  all_srenders_lock();
  sound_render_value(&buf, lOut, &am.l_clips, &am.l_peak,  &mc -> sr_left,  &mc -> fes_sr_left);
  sound_render_value(&buf, rOut, &am.r_clips, &am.r_peak,  &mc -> sr_right, &mc -> fes_sr_right);
  all_srenders_unlock();
 }

 return mc -> xr -> unpacked;
}

/* the end...
*/

