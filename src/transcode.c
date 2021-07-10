/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      transcode.c -- Transcoding .cwave to sound file via Winamp's
 *      format converter (XMPlay uses generic playback API)
 *      Lots of thanks to David Bryant, author of WavPack, www.wavpack.com --
 *      -- his sources of WinAmp plugin for WavPack playback is a single
 *      point, where we found WinAmp transcoding API.
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

#include "in_cwave.h"

/* open with check for transcoding
*/
__declspec(dllexport) intptr_t
#if defined(UNICODE)
        winampGetExtendedRead_openW
#else
        winampGetExtendedRead_open
#endif
        (const TCHAR *filename, int *size, int *bps, int *nch, int *srate)
{
 int64_t sz;
 MOD_CONTEXT *mc = &(the.mc_transcode);
 int out_size;

 if(!mod_context_fopen(filename, 0, mc))                // don't know about buffer size
  return 0;

 out_size = sound_render_size(&mc -> sr_left) + sound_render_size(&mc -> sr_right);
 sz = (int64_t)mc -> xr -> n_samples * (int64_t)(out_size);

 // ..significantly unsure..
 if(sz > (0x7FFFFFFFLL
        - sizeof(WAVEFORMATEXTENSIBLE)
        - 12LL /*RIFF<siz>WAVE*/
        - 16LL /* "fmt " + "data" chunks headers*/
        - 64LL /* for the some something */))
 {
  // probably, we can to truncate the out length here. but now we simple refuse to open so big file.
  mod_context_fclose(mc);
  return 0;
 }

 // output to Winamp parameters
 *nch = 2 /* OUT channels */;
 *srate = mc -> xr -> sample_rate;
 *bps = out_size * (8 / 2 /* OUT channels */);

 *size = (int)sz;

 return ((intptr_t)mc);
}

/* read and render a cwave (or WAV) data portion
*/
__declspec(dllexport) intptr_t winampGetExtendedRead_getData(intptr_t handle,
        char *dest, int len, int *killswitch)
{
// *dest -- buffer, sizeof(dest) == len -- in BYTES (not in samples!)
 MOD_CONTEXT *mc = (MOD_CONTEXT *)handle;
 unsigned out_size = sound_render_size(&mc -> sr_left) + sound_render_size(&mc -> sr_right);
 unsigned nsamples = ((unsigned)len) / out_size;
 unsigned real_bytes = 0;

 if(nsamples && !*killswitch)
 {
  xwave_change_read_quant(nsamples, mc -> xr);
  // sorry about *killswitch -- really buffer is about 4-64K bytes, so
  // we don't care about cancelation within this piece of processing
  real_bytes = amod_process_samples(dest, mc) * out_size;
 }

 return real_bytes;
}

/* seek the input for given time; TRUE if success, FALSE if FAIL
*/
__declspec (dllexport) int winampGetExtendedRead_setTime(intptr_t handle, int decode_pos_ms)
{
 MOD_CONTEXT *mc = (MOD_CONTEXT *)handle;

 return xwave_seek_ms(decode_pos_ms, mc -> xr);
}

/* close transcoding file
*/
__declspec(dllexport) void winampGetExtendedRead_close(intptr_t handle)
{
 MOD_CONTEXT *mc = (MOD_CONTEXT *)handle;

 mod_context_fclose(mc);
}

/* the End...
*/

