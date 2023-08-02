/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      gui_cwave.c -- GUI init/cleanup/minor dialog(s)
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

/* Initialization
 * --------------
 */
/* one-time module initialization
*/
BOOL gui_init(void)
{
 INITCOMMONCONTROLSEX icc;

 icc.dwSize = sizeof icc;
 icc.dwICC = ICC_BAR_CLASSES;
 if(InitCommonControlsEx(&icc))
 {
  amgui_init();
  return TRUE;
 }
 return FALSE;
}

/* module cleanup
*/
void gui_cleanup(void)
{
 amgui_cleanup();
}

/* keep window(s) alive while GUI thread busy
*/
void make_events(void)
{
 MSG msg;

 while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
 {
  TranslateMessage(&msg);       // probably not need
  DispatchMessage(&msg);
 }
}


/* The file info dialog
 * --- ---- ---- ------
 */
// the dialog helpers
/* check CWAVE data integrity
*/
static void check_cwave(HWND hwnd, int lb_id, XWAVE_READER *xr)
{
 static const TCHAR sz_errm[] = _T("** Read error or file corrupted, aborted...");
 int64_t tot_readed = 0;
 unsigned crc;

 LB_Printf(hwnd, lb_id, _T("-- CWAVE CRC Check, please wait..."));

 crc32init(&xr -> spec.cwave.cur_crc);
 if(!xwave_seek_samples(0, xr))
 {
  LB_Printf(hwnd, lb_id, sz_errm);
  return;
 }

 for(;;)
 {
  if(!xwave_read_samples(xr))
  {
   LB_Printf(hwnd, lb_id, sz_errm);
   return;
  }
  if(!xr -> really_readed)
   break;

  tot_readed += xr -> really_readed;
  crc32update(xr -> tbuff, xr -> really_readed * xr -> sample_size, &xr -> spec.cwave.cur_crc);
  make_events();                        // keep window(s) alive
 }

 crc = crc32final(&xr -> spec.cwave.cur_crc);
 if(xr -> spec.cwave.header.version > HCW_VERSION_V1)
 {
  if(crc == xr -> spec.cwave.header.n_CRC32)
  {
   LB_Printf(hwnd, lb_id, _T("-- Sample data CRC (0x%08X) OK!!!"), crc);
  }
  else
  {
   LB_Printf(hwnd, IDL_INFO, _T("** BAD CRC: 0x%08X, must be 0x%08X"),
        crc, xr -> spec.cwave.header.n_CRC32);
  }
 }
 else
 {
  LB_Printf(hwnd, lb_id, _T("-- Sample data CRC is 0x%08X, file did not contain CRC"), crc);
 }
}

/* print info about CWAVE file
*/
static void print_info_cwave(HWND hwnd, int lb_id, XWAVE_READER *xr)
{
 static const TCHAR *fmt_title[] =
 {
  _T("[Re/Im double (2*64 bits) samples, -32768.0..32767.0]"),
  _T("[Re/Im short (2*16 bits) samples, -32768..32767.0]"),
  _T("[Re/Im short+float (16+32 bits) samples, -32768..32767.0]"),
  _T("[Re/Im float (2*32 bits) samples, -32768.0..32767.0]")
 };

 LB_Printf(hwnd, lb_id, _T("-- Complex Wave File [CWAVE]:"));

 LB_Printf(hwnd, lb_id, xr-> file_pure_name);
 LB_Printf(hwnd, lb_id, xr-> file_full_name);

 LB_Printf(hwnd, lb_id, _T("Format version: %u"), xr -> spec.cwave.header.version);
 LB_Printf(hwnd, lb_id, _T("Number of channels: %u"), xr -> spec.cwave.header.n_channels);
 LB_Printf(hwnd, lb_id, _T("Number of samples: %u"), xr -> spec.cwave.header.n_samples);
 LB_Printf(hwnd, lb_id, _T("Sample rate: %u Hz"), xr -> spec.cwave.header.sample_rate);
 LB_Printf(hwnd, lb_id, _T("Sample format code: %u"), xr -> spec.cwave.header.format);

 LB_Printf(hwnd, lb_id, xr -> spec.cwave.header.format > HCW_FMT_PCM_FLT32?
        _T("** Unknown sample format")
        :
        fmt_title[xr -> spec.cwave.header.format]);

 if(xr -> spec.cwave.header.k_M > 0)
 {
  LB_Printf(hwnd, lb_id, _T("Created with the Hilbert's FIR: Order %u, Window param. %G"),
        xr -> spec.cwave.header.k_M, xr -> spec.cwave.header.k_beta);
 }
 else
 {
  LB_Printf(hwnd, lb_id,
        -1 == xr -> spec.cwave.header.k_M?
                _T("Created with the FFT (weak)")
                :
                _T("Created with unknown converter"));
 }

 if(xr -> spec.cwave.header.version > 1)
  LB_Printf(hwnd, lb_id, _T("CRC32 of sample data 0x%08X"), xr -> spec.cwave.header.n_CRC32);
 else
  LB_Printf(hwnd, lb_id, _T("CRC32 is not supported in this file version"));
}


/* print info about WAV (RWAVE) file
*/
static void print_info_rwave(HWND hwnd, int lb_id, XWAVE_READER *xr)
{
 static const TCHAR *fmt_title[] =
 {
  _T("[Unsigned integers 8 bits (byte)]"),
  _T("[Singned integers 16 bits, (short)]"),
  _T("[Singned integers 24 bits, (3 bytes)]"),
  _T("[Singned integers 32 bits, (long)]"),
  _T("[IEEE floats 32 bits, -1.0..1.0]")
 };

 LB_Printf(hwnd, lb_id, _T("-- WAV Real Wave File [RWAVE]:"));

 LB_Printf(hwnd, lb_id, xr-> file_pure_name);
 LB_Printf(hwnd, lb_id, xr-> file_full_name);

 LB_Printf(hwnd, lb_id, _T("Number of channels: %u"), xr -> n_channels);
 LB_Printf(hwnd, lb_id, _T("Number of samples: %u"), (unsigned)(xr -> n_samples));
 LB_Printf(hwnd, lb_id, _T("Sample rate: %u Hz"), xr -> sample_rate);

 LB_Printf(hwnd, lb_id, xr -> spec.rwave.format > HRW_FMT_FLOAT32?
        _T("** Unknown sample format")
        :
        fmt_title[xr -> spec.rwave.format]);

 LB_Printf(hwnd, lb_id, _T("Format tag: %u, Avg Bytes per sec: %u, Blk align: %u"),
        xr -> spec.rwave.header.Format.wFormatTag,
        xr -> spec.rwave.header.Format.nAvgBytesPerSec,
        xr -> spec.rwave.header.Format.nBlockAlign);

 switch(xr -> spec.rwave.htype)
 {
  case HRW_HTYPE_WFONLY:                                // WAVFORMAT ONLY (bad!)
   LB_Printf(hwnd, lb_id, _T("[WAVEFORMAT-only header (incorrect!)]"));
   LB_Printf(hwnd, lb_id, _T("!! Bits-per-sample computed by info from incomplete header"));
   break;

  case HRW_HTYPE_PCMW:                                  // PCMWAVEFORMAT, incl. float
   LB_Printf(hwnd, lb_id, _T("[PCMWAVEFORMAT header (correct up to PCM 16bits/2ch)]"));
   if(xr -> spec.rwave.format > HCW_FMT_PCM_INT16)
    LB_Printf(hwnd, lb_id, _T("!! This type of header type is obsolete for this data format"));
   break;

  case HRW_HTYPE_EXT:                                   // WAVEFORMATEXTENSIBLE, incl. float
   LB_Printf(hwnd, lb_id, _T("[WAVEFORMATEXTENSIBLE (GUID'ed) header]"));
   LB_Printf(hwnd, lb_id, _T("Additional data size %u bytes"),
        xr -> spec.rwave.header.Format.cbSize);
   LB_Printf(hwnd, lb_id, _T("Valid bits per sample: %u"),
        xr -> spec.rwave.header.Samples.wValidBitsPerSample);
   LB_Printf(hwnd, lb_id, _T("Channel mask: 0x%04X"),
        xr -> spec.rwave.header.dwChannelMask);
   break;

  default:
   LB_Printf(hwnd, lb_id, _T("** Unknown header type"));
   break;
 }

 LB_Printf(hwnd, lb_id, _T("-- Additional meta data (if any) ignored"));
}

/* print info about a file
*/
static BOOL print_info(HWND hwnd, int lb_id, XWAVE_READER *xr)  // TRUE, if checkable
{
 BOOL checkable = FALSE;

 switch(xr -> type)
 {
  case XW_TYPE_CWAVE:
   print_info_cwave(hwnd, lb_id, xr);
   checkable = TRUE;                                    // always chackable now
   break;

  case XW_TYPE_RWAVE:
   print_info_rwave(hwnd, lb_id, xr);
   break;
 }

 return checkable;
}

/* Message handlers
 * ------- --------
 */
/* dialog initialization
*/
static BOOL Info_Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
 XWAVE_READER *xr = (XWAVE_READER *)lParam;

 SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)xr);
 LB_SetHscroll(hwnd, IDL_INFO, MAX_FILE_PATH /*?unsure*/);

 // IDC_SHOWPLAY -- in the INVERSE logic here!!
 CheckDlgButton(hwnd, IDC_SHOWPLAY, getShowPlay()? BST_UNCHECKED : BST_CHECKED);

 EnableWindow(GetDlgItem(hwnd, IDB_CHECK), print_info(hwnd, IDL_INFO, xr));

 return TRUE;                                           // default focus
}

/* command handling
*/
static void Info_Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
 XWAVE_READER *xr = (XWAVE_READER *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

 if(!xr)
  return;

 switch(id)
 {
  // toggle "ALT+3"
  case IDC_SHOWPLAY:
   // IDC_SHOWPLAY -- in the INVERSE logic here!!
   setShowPlay(!!(BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_SHOWPLAY)));
   break;

  // CRC check:
  case IDB_CHECK:
   EnableWindow(GetDlgItem(hwnd, IDB_CHECK), FALSE);
   EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
   EnableWindow(GetDlgItem(hwnd, IDL_INFO), FALSE);

   check_cwave(hwnd, IDL_INFO, xr);

   EnableWindow(GetDlgItem(hwnd, IDL_INFO), TRUE);
   EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
   EnableWindow(GetDlgItem(hwnd, IDB_CHECK), TRUE);
   break;

  // termination
  case IDCANCEL:                // exit dialog -- free all
  case IDOK:                    // -"-
   EndDialog(hwnd, 0);
   break;
 }
}

/* the main dialog function
*/
static BOOL CALLBACK Info_DlgProc(HWND hDlg, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
 BOOL fProcessed = TRUE;        // FALSE for WM_INITDIALOG if call SetFocus

 switch(uMsg)                   // message handling
 {
  HANDLE_MSG(hDlg, WM_INITDIALOG,       Info_Dlg_OnInitDialog);
  HANDLE_MSG(hDlg, WM_COMMAND,          Info_Dlg_OnCommand);
  default:
   fProcessed = FALSE;
   break;
 }
 return fProcessed;
}

/* the front-end for file info dialog
*/
void gui_info_dialog(HINSTANCE hi, HWND hwndParent, XWAVE_READER *xr)
{
 if(xr)
 {
  DialogBoxParam(hi, MAKEINTRESOURCE(IDD_INFO), hwndParent, Info_DlgProc, (LPARAM)xr);
 }
}


/* the end...
*/

