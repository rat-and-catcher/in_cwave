/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      hcontrols.h -- generic Windows controls handling
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

#if !defined(_hcontrols_h_)
#define _hcontrols_h_

#include "compatibility/compat_gcc.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>

#include "compatibility/compat_win32_gcc.h"

#include "cmalloc.h"

#if defined(__cplusplus)
extern "C" {
#endif

// max line length to print ((w)chars)
#define MAX_PRINT_LEN   (65536)                         /* huge enough */

// max symbols in input line ((w)chars)
#define MAX_INPUT_LEN   (1024)                          /* huge enough; on stack */

/* Common inlines
 * ------ -------
 */
/* common Enable/Disable method
*/
static __inline void EnDis(HWND hWnd, int id, BOOL qen)
{
 EnableWindow(GetDlgItem(hWnd, id), qen);
}

/* Track Bar Operations
 * ----- --- ----------
 */
/* set the range of track bar
*/
void TB_SetRange(HWND hWnd, int id, LONG maximum, LONG minimum, BOOL fRedraw);
/* set the steps of track bar
*/
void TB_SetSteps(HWND hWnd, int id, LONG linesize, LONG pagesize);
/* set tick frequency
*/
void TB_SetTicks(HWND hWnd, int id, LONG freq);
/* set track position
*/
void TB_SetTrack(HWND hWnd, int id, LONG pos, BOOL fRedraw);
/* get track position
*/
LONG TB_GetTrack(HWND hWnd, int id);

/* Edit box / static text
 * ---- --- - ------ ----
 */
/* set text in a printf manner
*/
void TXT_PrintTxt(HWND hWnd, int id, const TCHAR *fmt, ...);
/* get the text from the control
*/
TCHAR *TXT_GetTxt(HWND hWnd, int id, TCHAR *buf, int maxlen);
/* get integer from the control (min >= max -> unchecked)
*/
LONG TXT_GetLng(HWND hWnd, int id, LONG vdef, LONG vmin, LONG vmax);
/* get unsigned from the control (min >= max -> unchecked)
*/
ULONG TXT_GetUlng(HWND hWnd, int id, ULONG vdef, ULONG vmin, ULONG vmax);
/* get double from the control (min >= max -> unchecked)
*/
double TXT_GetDbl(HWND hWnd, int id, double vdef, double vmin, double vmax, const TCHAR *prfmt);

/* List box
 * ---- ---
 */
/* set horizontal scroll extent in pixels
*/
void LB_SetHscroll(HWND hWnd, int id, int ext_hsize);
/* print a line to list box
*/
void LB_Printf(HWND hWnd, int id, const TCHAR *fmt, ...);
/* Clear list box
*/
void LB_ClearAll(HWND hWnd, int id);
/* set current selection of list
*/
int LB_SetSel(HWND hWnd, int id, int ix);
/* get current list index and data
*/
int LB_GetIndexRec(HWND hWnd, int id, LRESULT *param);
/* add to the end of list string with associated data
*/
int LB_AddRecord(HWND hWnd, int id, LPCTSTR str, LPARAM param);
/* remove the last item from list
*/
int LB_DelRecord(HWND hWnd, int id);

/* Combo box
 * ----- ---
 */
/* Clear combo box
*/
void CB_ClearAll(HWND hWnd, int id);
/* set current selection of combo box list
*/
int CB_SetSel(HWND hWnd, int id, int ix);
/* get current list index and data
*/
int CB_GetIndexRec(HWND hWnd, int id, LRESULT *param);
/* add to the end of list string with associated data
*/
int CB_AddRecord(HWND hWnd, int id, LPCTSTR str, LPARAM param);
/* remove the last item from combo box list
*/
int CB_DelRecord(HWND hWnd, int id);


#if defined(__cplusplus)
}
#endif

#endif                  // def _hcontrols_h_

/* the end...
*/

