/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      iwa_ipc.c -- some Winamp IPC stuff
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

#include "iwa_ipc.h"

// We unsure about this interface -- what of the functions are really need
// and how them can help to close some common logic holes in our interactions
// with Winamp. So, the file may look somewhat odd. sorry.
// Note, that the routines are completely thread unsafe.

/* one and only one IPC object -- internal
*/
static struct
{
 HWND wa_win;                       // Winamp window to send IPC messages
 int ver_major;                     // Winamp version major (1..5.. etc), 0 == uninialized
 int ver_minor;                     // Winamp version minor (0,1,2,..10,11,..66,..)
 int is_dis_exit;                   // we disable Winamp exit via IPC_PUSH_DISABLE_EXIT
} wa_ipc =
{ NULL, 0, 0 };                     // uninitialized

/* initialize module -- ask Winamp version, 0 == OK
*/
int iwa_init(HWND wa_hwnd)
{
 int res = 0;
 LRESULT wa_ver;

 if(wa_hwnd)                        // XMPlay protection -- IPC does not work with XMPlay
 {
  SetLastError(ERROR_SUCCESS);
  wa_ver = SendMessageW(wa_hwnd, WM_WA_IPC, 0, IPC_GETVERSION);
 }

 if(NULL == wa_hwnd || ERROR_SUCCESS != GetLastError())
 {
  // if we can't to exec IPC_GETVERSION with success, block the interface
  res = -1;
  memset(&wa_ipc, 0, sizeof(wa_ipc));
 }
 else
 {
  wa_ipc.wa_win         = wa_hwnd;
  wa_ipc.ver_major      = WINAMP_VERSION_MAJOR(wa_ver);
  wa_ipc.ver_minor      = WINAMP_VERSION_MINOR(wa_ver);
  wa_ipc.is_dis_exit    = 0;
 }

 return res;                        // can be ignored by the host
}

/* return Winamp version, 0 == OK
*/
int iwa_version(int *ver_mj, int *ver_mn)
{
 if(wa_ipc.wa_win)
 {
  if(ver_mj)
   *ver_mj = wa_ipc.ver_major;
  if(ver_mn)
   *ver_mn = wa_ipc.ver_minor;

  return 0;
 }

 return -1;                         // not initialized
}

/* weak disable Winamp exit, 0 == OK
*/
int iwa_dexit(void)
{
 if(wa_ipc.wa_win)
 {
  if(wa_ipc.ver_major >= 5 && wa_ipc.ver_minor >= 4)
  {
   if(!wa_ipc.is_dis_exit)
   {
    // it's not very critical, that something goes wrong now; so we dont check the IPC for the fail
    LRESULT now = SendMessageW(wa_ipc.wa_win, WM_WA_IPC, 0, IPC_IS_EXIT_ENABLED);
    if(now)
    {
     (void)SendMessageW(wa_ipc.wa_win, WM_WA_IPC, 0, IPC_PUSH_DISABLE_EXIT);
     wa_ipc.is_dis_exit = 1;
    }
   }
  }

  return 0;
 }

 return -1;
}

/* (re-)enable Winamp exit, 0 == OK
*/
int iwa_eexit(void)
{
 if(wa_ipc.wa_win)
 {
  if(wa_ipc.ver_major >= 5 && wa_ipc.ver_minor >= 4)
  {
   if(wa_ipc.is_dis_exit)
   {
    // here we don't check real Winamp exit state
    (void)SendMessageW(wa_ipc.wa_win, WM_WA_IPC, 0, IPC_POP_DISABLE_EXIT);
    wa_ipc.is_dis_exit = 0;
   }
  }

  return 0;
 }

 return -1;
}


/* the end...
*/

