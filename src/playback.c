/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      playback.c -- the main playback interface
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

#include "in_cwave.h"

/* Forwards
 * --------
 */
/* the decode thread procedure
*/
static DWORD WINAPI DecodeThread(LPVOID context);

/* internal playback type(s)
*/
// playback plugin context
typedef struct tagPLUGIN_CONTEXT
{
 char *play_sample_buffer;              // playback sample buffer -- twice as big as the block size
                                        // (block size == NS_PERTIME (* sizeof(int) * max.out.channels)
 int decode_pos_ms;                     // current decoding position, in milliseconds.
                                        // Used for correcting DSP plug-in pitch changes
 int paused;                            // are we paused?
 volatile int seek_needed;              // if != -1, it is the point that the decode
                                        // thread should seek to, in ms.
 volatile int kill_decode_thread;       // the kill switch for the playback thread
 HANDLE thread_handle;                  // the handle for the decode thread
// GUI state(s)
 BOOL is_gui_init;                      // if FALSE -- GUI stuff unavailable
 BOOL show_play;                        // show setup by ALT+3
// flag to prevent to run multiply copy of GUI
 volatile LONG is_gui_run;              // for InterlockedCompareExchange()

 TCHAR *last_file_name;                 // the last file name was open for playback
 int last_length_in_ms;                 // the last file length in ms was open for playback
} PLUGIN_CONTEXT;

/* Global Statics
 * ------ -------
 */
static PLUGIN_CONTEXT plg;              // playback interface context + thread control
static In_Module pb_iface;              // the in_plugin interface (filled in near the bottom of this file)

/* Winamp In Callbacks
 * ------ -- ---------
 */
/* configuration
*/
static void config(HWND hwndParent)
{
 // avoid to run multiply config dialog copies
 // (need for work as XMPlay Player plugin)
 if(0 == InterlockedCompareExchange(&plg.is_gui_run, 1, 0))
 {
  plg.is_gui_run = 1;
  if(plg.is_gui_init)
  {
   amgui_setup_dialog(pb_iface.hDllInstance, hwndParent);
  }
  else
  {
   MessageBox(hwndParent
        , _T("Configuration dialog not initialized")
        , _T("CWAVE quad modulator error")
        , MB_OK | MB_ICONWARNING);
  }

  plg.is_gui_run = 0;
 }
}

/* "about" box
*/
static void about(HWND hwndParent)
{
 const TCHAR fmsg[] =
        _T("CWAVE / WAV ('RWAVE') universal reader / quad modulator\n")
        _T("Version ")
        _T(VERSION_IN_CWAVE)
        _T("\n")
        _T("Config version ID is %u\n")
        _T("The plugin released under the BSD-like license\n")
        _T("USE IT AT YOUR OWN RISC");
 TCHAR msg[sizeof(fmsg) / sizeof(TCHAR) + 80 + 2];

 int safez = _sntprintf(msg, sizeof(fmsg) / sizeof(TCHAR) + 80, fmsg, the.cfg.ver_config);

 // to avoid silly code analizies warning
 msg[sizeof(msg) / sizeof(TCHAR) - 1 > safez? safez : sizeof(msg) / sizeof(TCHAR) - 1] = _T('\0');

 MessageBox(hwndParent, msg, _T("About CWAVE quad modulator"), MB_OK | MB_ICONINFORMATION);
}

/* any one-time initialization goes here (configuration reading, etc)
*/
static void init(void)
{
 // 'the' already initialized; so -- plugin context
 memset(&plg, 0, sizeof(plg));

 // CanWrite() returns the number of bytes you can write, so we check that
 // to the block size. the reason we multiply the block size by two if
 // pb_iface.dsp_isactive() is that DSP plug-ins can change it by up to a
 // factor of two (for tempo adjustment).
 // So, sample buffer twice as big as the block size
 plg.play_sample_buffer = cmalloc(NS_PERTIME * sizeof(int) * /*max.out.ch*/ 2 * /*twice*/ 2);
 plg.decode_pos_ms = 0;
 plg.paused = 0;
 plg.seek_needed = -1;
 plg.kill_decode_thread = 0;
 plg.thread_handle = NULL;
 plg.is_gui_init = gui_init();
 plg.show_play = TRUE;
 plg.is_gui_run = 0;
 plg.last_file_name = NULL;
 plg.last_length_in_ms = -1000 /* default unknown */;
}

/* one-time deinitialization, such as memory freeing
*/
static void quit()
{
 gui_cleanup();

 if(plg.last_file_name)
 {
  free(plg.last_file_name);
  plg.last_file_name = NULL;
 }

 plg.last_length_in_ms = -1000 /* default unknown */;

 if(plg.play_sample_buffer)
 {
  free(plg.play_sample_buffer);
  plg.play_sample_buffer = NULL;
 }
 module_cleanup();                                      // init in winampGetInModule2()
}

/* used for detecting URL streams... unused here.
*/
static int isourfile(const TCHAR *filename)
{
// return !strncmp(fn,"http://",7); to detect HTTP streams, etc
 return 0;
}

/* called when Winamp wants to play a file
*/
static int play(const TCHAR *filename)
{
 int maxlatency;
 unsigned thread_id;
 unsigned out_size;
 MOD_CONTEXT *mc = &(the.mc_playback);

 plg.paused = 0;
 plg.decode_pos_ms = 0;
 plg.seek_needed = -1;

 // Open our file here
 if(!mod_context_fopen(filename, NS_PERTIME, mc))
 {
  // we return error. 1 means to keep going in the playlist, -1
  // means to stop the playlist.
  return 1;
 }

 // update last_file_name and last_length_in_ms for WinAmp odds
 if(plg.last_file_name)
 {
  free(plg.last_file_name);
  plg.last_file_name = NULL;
 }
 plg.last_file_name =
    cmalloc((_tcslen(mc -> xr -> file_pure_name) + 1) * sizeof(TCHAR));
 _tcscpy(plg.last_file_name, mc -> xr -> file_pure_name);
 plg.last_length_in_ms =
    (int)((xwave_get_nsamples(mc -> xr) * 1000LL) / mc -> xr -> sample_rate);

 out_size = sound_render_size(&mc -> sr_left) + sound_render_size(&mc -> sr_right);

 // -1 and -1 are to specify buffer and prebuffer lengths.
 // -1 means to use the default, which all input plug-ins should
 // really do.
 maxlatency = pb_iface.outMod -> Open(mc -> xr -> sample_rate,
        2 /* OUT channels */,
        (out_size << (3 /* bits in byte */ - 1 /* / 2 chamnnels */)),
        -1, -1);

 // maxlatency is the maximum latency between a outMod -> Write() call and
 // when you hear those samples. In ms. Used primarily by the visualization
 // system.
 if(maxlatency < 0)                     // error opening device
 {
  mod_context_fclose(mc);
  return 1;
 }

 // dividing by 1000 for the first parameter of setinfo makes it
 // display 'H'... for hundred.. i.e. 14H Kbps.
 pb_iface.SetInfo((mc -> xr -> sample_rate * 8 /*bits in byte*/) / 1000 * out_size,
        mc -> xr -> sample_rate / 1000,
        2 /* OUT channels */,
        1);

 // initialize visualization stuff
 pb_iface.SAVSAInit(maxlatency, mc -> xr -> sample_rate);
 pb_iface.VSASetInfo(mc -> xr -> sample_rate, 2 /* OUT channels */);

 // set the output plug-ins default volume.
 // volume is 0-255, -666 is a token for
 // current volume.
 pb_iface.outMod -> SetVolume(-666);

 // launch decode thread
 plg.kill_decode_thread = 0;
 plg.thread_handle = (HANDLE)_beginthreadex(NULL,
        0,
        (unsigned (WINAPI *) (void *))DecodeThread,
        &plg,                           // thread context
        0,
        &thread_id);

 return 0;
}

// standard pause implementation
/* pause playback
*/
static void pause(void)
{
 plg.paused = 1;
 pb_iface.outMod -> Pause(1);
}

/* resume playback
*/
static void unpause(void)
{
 plg.paused = 0;
 pb_iface.outMod -> Pause(0);
}

/* test for paused state
*/
static int ispaused(void)
{
 return plg.paused;
}

/* stop playing
*/
static void stop(void)
{
 MOD_CONTEXT *mc = &(the.mc_playback);

 if(NULL != plg.thread_handle)
 {
  plg.kill_decode_thread = 1;
  if(WaitForSingleObject(plg.thread_handle, 10000) == WAIT_TIMEOUT)      // 10" pretty enough
  {
   // ureachable (according WinAmp's SDK here must lay the MessageBox();
   // there is no way to continue)
   FatalAppExit(0, _T("Error asking internal in_cwave.dll thread to die!\n")
        _T("This is seriously problem. Player (probably the whole system)\n")
        _T("need to be close :,((\n"));
   // ureachable ** 2. If we keep TerminateThread(), this turn down
   // the plugin and the player into unpredicable state.

#if defined(_MSC_VER)
// just for Microsotf static code analizer
#pragma warning(suppress: 6258)
#endif

   // TODO:: this seems need for complete removal
   TerminateThread(plg.thread_handle, 0);
  }
  CloseHandle(plg.thread_handle);
  plg.thread_handle = NULL;
 }

 // close output system
 pb_iface.outMod -> Close();

 // deinitialize visualization
 pb_iface.SAVSADeInit();

 // destroy the renderd and close input file (if any) -- it's safe now
 // (if no TerminateThread() to call, it's already closed by thread cleanup,
 // but multiply calls is safe)
 mod_context_fclose(mc);
}

/* returns length of playing track, ms
*/
static int getlength(void)
{
 int64_t t;
 MOD_CONTEXT *mc = &(the.mc_playback);

 mp_playback_lock();
 t = mc -> xr                           // original flag 'isPlayback'
        // check of sample_rate != 0 from V1.4.1-; lets it be alive fo some time...
        // (Are you sure about sample_rate? Today it checked for zero in XWAVE_READER;
        // tomorow may be not...)
        && mc -> xr -> sample_rate?
        (xwave_get_nsamples(mc -> xr) * 1000LL) / mc -> xr -> sample_rate
        :
        -1000;                          // default unknown file len
 mp_playback_unlock();

 return (int)t;
}


/* returns current output position, in ms.
*/
static int getoutputtime(void)
{
// we could just use return pb_iface.outMod->GetOutputTime(),
// but the DSP plug-ins that do tempo changing tend to make
// that wrong.
 return plg.decode_pos_ms +
        (pb_iface.outMod -> GetOutputTime() - pb_iface.outMod -> GetWrittenTime());
}


/* set need seek in ms
*/
static void setoutputtime(int time_in_ms)
{
// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
 plg.seek_needed = time_in_ms;
}

// standard volume/pan functions
/* set the volume
*/
static void setvolume(int volume)
{
 pb_iface.outMod -> SetVolume(volume);
}

/* set the pan
*/
static void setpan(int pan)
{
 pb_iface.outMod -> SetPan(pan);
}

/* start File Info dialog
*/
static int infoBox(const TCHAR *filename, HWND hwnd)
{
// this gets called when the use hits Alt+3 to get the file info.
 if(!plg.is_gui_init)
  return 0;

 // avoid to run multiply config dialog copies
 // (need for work as XMPlay Player plugin)
 // NOTE::really this only partially work. When user try to get file info
 // for multiply files selection, the function will be serially called
 // for each file, and new config / info window will appear immediatly
 // after closing the current one -- as many times as the number of selected files.
 // We can't avoid this.
 if(0 == InterlockedCompareExchange(&plg.is_gui_run, 1, 0))
 {
  HWND par_wnd = hwnd;

  plg.is_gui_run = 1;

  switch(the.cfg.infobox_parenting)
  {
   case INFOBOX_NOPARENT:
    par_wnd = NULL;
    break;
   case INFOBOX_LISTPARENT:
    par_wnd = hwnd;
    break;
   case INFOBOX_MAINPARENT:
    par_wnd = pb_iface.hMainWindow;
    break;
  }

  if(plg.show_play)
  {
   amgui_setup_dialog(pb_iface.hDllInstance, par_wnd);
  }
  else
  {
   // create w/o any aligment or fading for info/CRC check
   XWAVE_READER *xr = xwave_reader_create(filename, NS_CHECKPT, 0, 0, 0);

   if(xr)
   {
    gui_info_dialog(pb_iface.hDllInstance, par_wnd, xr);
    xwave_reader_destroy(xr);
    xr = NULL;
   }
   else
   {
    MessageBox(hwnd,
        filename,
        _T("The file is corrupted or locked by another application:"),
        MB_OK | MB_ICONSTOP | MB_APPLMODAL);
   }
  }

  plg.is_gui_run = 0;
 }
 return 0;
}

/* get title and/or length of track
*/
static void getfileinfo(const TCHAR *filename, TCHAR *title, int *length_in_ms)
{
// this is an odd function. it is used to get the title and/or
// length of a track.
// if filename is either NULL or of length 0, it means you should
// return the info of lastfn. Otherwise, return the information
// for the file in filename.
// if title is NULL, no title is copied into it.
// if length_in_ms is NULL, no length is copied into it.
 MOD_CONTEXT *mc = &(the.mc_playback);

 if(!filename || !*filename)            // currently playing file
 {
  // ..it's look somewhat buggy..
  mp_playback_lock();
  if(length_in_ms)                      // code from getlength() -- need common lock
   *length_in_ms = mc -> xr && mc -> xr -> sample_rate?
        (int)((xwave_get_nsamples(mc -> xr) * 1000LL) / mc -> xr -> sample_rate)
        :
        (plg.last_file_name?
            plg.last_length_in_ms
            :
            -1000/* default unknown file len */);

  if(title)                             // get non-path portion of filename
  {
   _tcscpy(title
    , mc -> xr?
        mc -> xr -> file_pure_name
        :
        (plg.last_file_name? plg.last_file_name : _T("-EMPTY-")));
  }
  mp_playback_unlock();
 }
 else                                   // some other file
 {
  XWAVE_READER *xr = xwave_reader_create(
      filename
    , 0
    , the.cfg.sec_align
    , the.cfg.fade_in
    , the.cfg.fade_out
    );

  if(xr)
  {
   if(length_in_ms)
   {
    int64_t t = (xwave_get_nsamples(xr) * 1000LL) / xr -> sample_rate;
    *length_in_ms = (int)t;
   }
   if(title)
    _tcscpy(title, xr -> file_pure_name);

   xwave_reader_destroy(xr);
   xr = NULL;
  }
  else                                  // can't open
  {
   if(length_in_ms)
    *length_in_ms = -1000;
   if(title)
    _tcscpy(title, _T("-CAN'T OPEN-"));
  }
 }
}

/* standard EQ setting
*/
static void eq_set(int on, char data[10], int preamp)
{
// most plug-ins can't even do an EQ anyhow.. I'm working on writing
// a generic PCM EQ, but it looks like it'll be a little too CPU
// consuming to be useful :)
// if we _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
// and preamp is the same.
}


/* HELPERS AND THREADS
 * ------- --- -------
 */
/* show setup by ALT+3
*/
BOOL getShowPlay(void)
{
 return plg.show_play;
}
void setShowPlay(BOOL nsp)
{
 plg.show_play = nsp;
}

/* worker (decoder) thread function
*/
static DWORD WINAPI DecodeThread(LPVOID context)
{
 PLUGIN_CONTEXT *pc = (PLUGIN_CONTEXT *)context;
 MOD_CONTEXT *mc = &(the.mc_playback);

 int done = 0;                          // set to TRUE if decoding has finished
 unsigned out_size = sound_render_size(&mc -> sr_left) + sound_render_size(&mc -> sr_right);

 while (!pc -> kill_decode_thread)
 {
  if(pc -> seek_needed != -1)           // seek is needed.
  {
   pc -> decode_pos_ms = pc -> seek_needed;
   pc -> seek_needed = -1;
   done = 0;
   pb_iface.outMod -> Flush(pc -> decode_pos_ms); // flush output device and set  output pos. to the seek pos.

   // decode_pos_ms * SAMPLERATE / 1000 -- all in xwave_seek_ms():
   // we do hope, that (accordig generel tests) seek don't fail
   (void)xwave_seek_ms(pc -> decode_pos_ms, mc -> xr);

   // and reset the playback Hilbert's converter
   mod_context_reset_hilbert(mc);
  }

  if(done)                              // done was set to TRUE during decoding, signaling EOF
  {
   pb_iface.outMod -> CanWrite();       // some out drivers need CanWrite to be called on a regular basis
   if(!pb_iface.outMod -> IsPlaying())
   {
    // we're done playing, so tell Winamp and quit the thread.
    PostMessage(pb_iface.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
    break;                              // to shutdown the thread
   }
   SleepEx(10, TRUE);                   // give a little CPU time back to the system.
  }
  else
  {
   if(pb_iface.outMod -> CanWrite() >=
        (int)(mc -> xr -> read_quant * out_size * (pb_iface.dsp_isactive()? 2 : 1)))
   // CanWrite() returns the number of bytes you can write, so we check that
   // to the block size. the reason we multiply the block size by two if
   // pb_iface.dsp_isactive() is that DSP plug-ins can change it by up to a
   // factor of two (for tempo adjustment).
   {
    int len;
    // retrieve samples
    len = amod_process_samples(pc -> play_sample_buffer, mc);
    if(!len)                            // no samples means we're at eof
    {
     done = 1;
     // according to WinAmp src@github, we should to do this here:
     pb_iface.outMod -> Write(NULL, 0);
    }
    else                                // we got samples!
    {
     if(len >= NS_PERTIME)              // some protector
     {
      // int timestamp = pb_iface.outMod -> GetWrittenTime();   // Winamp recomended...

      // give the samples to the vis subsystems
      pb_iface.SAAddPCMData(pc -> play_sample_buffer, 2 /* OUT channels */,
        out_size << (3 - 1 /*ONE ch bps*/), pc -> decode_pos_ms /* timestamp */);
      pb_iface.VSAAddPCMData(pc -> play_sample_buffer, 2 /* OUT channels */,
        out_size << (3 - 1 /*ONE ch bps*/), pc -> decode_pos_ms /* timestamp */);
     }

     // adjust decode position variable
     pc -> decode_pos_ms += (mc -> xr -> really_readed * 1000) / mc -> xr -> sample_rate;

     // if we have a DSP plug-in, then call it on our samples [???]
     if(pb_iface.dsp_isactive())
      len = pb_iface.dsp_dosamples((short *)pc -> play_sample_buffer, len,
        out_size << 3, 2 /* OUT channels */, mc -> xr -> sample_rate);

     // write the PCM data to the output system
     pb_iface.outMod -> Write(pc -> play_sample_buffer, len * out_size);
    }
   }
   else
   {
    // if we can't write data, wait a little bit. Otherwise, continue
    // through the loop writing more data (without sleeping)
    playback_sleep();
   }                                                            // if can write
  }                                                             // if done / write
 }                                                              // while(!kill)

 // normal playback reader cleanup
 mp_playback_lock();
 mod_context_fclose(mc);
 mp_playback_unlock();

 _endthreadex(0);
 return 0;
}

// playback interface module definition.
static In_Module pb_iface =
{
        IN_VER,                 // defined in IN2.H; contain UNICODE info
        "CWAVE/WAV ('RWAVE') Decoder/Modulator " VERSION_IN_CWAVE " "
#if defined(UNICODE)
        "(Unicode) "
#endif
        // Winamp runs on both alpha systems and x86 ones. :)
#if defined(__alpha)
        "(AXP)"
#else
        "(x86)"
#endif
        ,
        0,                      // hMainWindow (filled in by Winamp)
        0,                      // hDllInstance (filled in by Winamp)
// the next is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
//      NULL,                   // fill this in make_exts_list BEFORE RETURN FROM winampGetInModule2()
"CWAVE\0CWAVE (complex) Audio File (*.cwave)\0"
"WAV\0WAV (uncompressed PCM int/float only WAV) Audio File (*.wav)\0",
        1,                      // is_seekable
        1,                      // uses output plug-in system
        config,
        about,
        init,
        quit,
        getfileinfo,
        infoBox,
        isourfile,
        play,
        pause,
        unpause,
        ispaused,
        stop,

        getlength,
        getoutputtime,
        setoutputtime,

        setvolume,
        setpan,

        0, 0, 0, 0, 0, 0, 0, 0, 0, // visualization calls filled in by Winamp

        0, 0,                   // DSP calls filled in by Winamp

        eq_set,

        NULL,                   // setinfo call filled in by Winamp

        0                       // out_mod filled in by Winamp

};

/* Return In_Module interface
*/
In_Module *get_playback_iface(void)
{
 return &pb_iface;
}

/* make the .FileExtensions list for playback interface
*/
void make_exts_list(BOOL is_wav, BOOL is_rwave)
{
 static const char cwave_ext[] = "CWAVE\0CWAVE (complex) Audio File (*.cwave)";
 static const char   wav_ext[] = "WAV\0WAV (uncompressed PCM int/float only WAV) Audio File (*.wav)";
 static const char rwave_ext[] = "RWAVE\0RWAVE (PCM int/float WAV synonym) Audio File (*.rwave)";
 static const char last_zero[] = "";

 static char module_exts[sizeof(last_zero)
        + sizeof(cwave_ext)
        + sizeof(wav_ext)
        + sizeof(rwave_ext)];

 size_t offset = sizeof(cwave_ext);

 memcpy(module_exts, cwave_ext, sizeof(cwave_ext));
 if(is_wav)
 {
  memcpy(module_exts + offset, wav_ext, sizeof(wav_ext));
  offset += sizeof(wav_ext);
 }
 if(is_rwave)
 {
  memcpy(module_exts + offset, rwave_ext, sizeof(rwave_ext));
  offset += sizeof(rwave_ext);
 }
 // ..to be continue -- probably..

 memcpy(module_exts + offset, last_zero, sizeof(last_zero));
 pb_iface.FileExtensions = module_exts;
}


/* the end...
*/

