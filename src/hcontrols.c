/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      hcontrols.c -- generic Windows controls handling
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

#include "hcontrols.h"

/* Track Bar Operations
 * ----- --- ----------
 */

/* set the range of track bar
*/
void TB_SetRange(HWND hWnd, int id, LONG maximum, LONG minimum, BOOL fRedraw)
{
 HWND tb_hwnd = GetDlgItem(hWnd, id);

 SendMessage(tb_hwnd, (UINT)TBM_SETRANGEMIN,
        (WPARAM)(BOOL)FALSE, (LPARAM)minimum);
 SendMessage(tb_hwnd, (UINT)TBM_SETRANGEMAX,
        (WPARAM)(BOOL)fRedraw, (LPARAM)maximum);
}

/* set the steps of track bar
*/
void TB_SetSteps(HWND hWnd, int id, LONG linesize, LONG pagesize)
{
 HWND tb_hwnd = GetDlgItem(hWnd, id);

 SendMessage(tb_hwnd, (UINT)TBM_SETPAGESIZE,
        (WPARAM)0, (LPARAM)pagesize);
 SendMessage(tb_hwnd, (UINT)TBM_SETLINESIZE,
        (WPARAM)0, (LPARAM)linesize);
}

/* set tick frequency
*/
void TB_SetTicks(HWND hWnd, int id, LONG freq)
{
 SendMessage(GetDlgItem(hWnd, id), (UINT)TBM_SETTICFREQ,
        (WPARAM)(WORD)freq, (LPARAM)0);
}

/* set track position
*/
void TB_SetTrack(HWND hWnd, int id, LONG pos, BOOL fRedraw)
{
 SendMessage(GetDlgItem(hWnd, id), (UINT)TBM_SETPOS,
        (WPARAM)(BOOL)fRedraw, (LPARAM)pos);
}

/* get track position
*/
LONG TB_GetTrack(HWND hWnd, int id)
{
 LONG res = SendMessage(GetDlgItem(hWnd, id), (UINT)TBM_GETPOS,
                (WPARAM)0, (LPARAM)0);
 return res;
}

/* Edit box / static text
 * ---- --- - ------ ----
 */

/* set text in a printf manner
*/
void TXT_PrintTxt(HWND hWnd, int id, const TCHAR *fmt, ...)
{
 TCHAR buf[MAX_INPUT_LEN+1];    // enough?!
 va_list args;
 va_start(args, fmt);

 if(-1 == _vsntprintf(buf, MAX_INPUT_LEN, fmt, args))
 buf[MAX_INPUT_LEN] = _T('\0');
 SetWindowText(GetDlgItem(hWnd, id), buf);
 va_end(args);
}

/* get the text from the control
*/
TCHAR *TXT_GetTxt(HWND hWnd, int id, TCHAR *buf, int maxlen)
{
 *buf = _T('\0');               // just in case
 Edit_GetText(GetDlgItem(hWnd, id), buf, maxlen - 1);
 buf[maxlen - 1] = _T('\0');    // just in case
 return buf;
}

/* get integer from the control (min >= max -> unchecked)
*/
LONG TXT_GetLng(HWND hWnd, int id, LONG vdef, LONG vmin, LONG vmax)
{
 TCHAR buf[MAX_INPUT_LEN+1];    // enough?!
 LONG res;
 TCHAR dummy;

 TXT_GetTxt(hWnd, id, buf, MAX_INPUT_LEN);
 if(1 != _stscanf(buf, _T("%ld %c"), &res, &dummy))
 {                              // garbage
  res = vdef;
 }
 if(vmax > vmin)
 {
  if(res < vmin)
   res = vmin;
  if(res > vmax)
   res = vmax;
 }

 TXT_PrintTxt(hWnd, id, _T("%ld"), res);

 return res;
}

/* get unsigned from the control (min >= max -> unchecked)
*/
ULONG TXT_GetUlng(HWND hWnd, int id, ULONG vdef, ULONG vmin, ULONG vmax)
{
 TCHAR buf[MAX_INPUT_LEN+1];    // enough?!
 ULONG res;
 TCHAR dummy;

 TXT_GetTxt(hWnd, id, buf, MAX_INPUT_LEN);
 if(1 != _stscanf(buf, _T("%lu %c"), &res, &dummy))
 {                              // garbage
  res = vdef;
 }
 if(vmax > vmin)
 {
  if(res < vmin)
   res = vmin;
  if(res > vmax)
   res = vmax;
 }

 TXT_PrintTxt(hWnd, id, _T("%ld"), res);

 return res;
}

/* get double from the control (min >= max -> unchecked)
*/
double TXT_GetDbl(HWND hWnd, int id, double vdef, double vmin, double vmax, const TCHAR *prfmt)
{
 TCHAR buf[MAX_INPUT_LEN+1];    // enough?!
 double res;
 TCHAR dummy;

 TXT_GetTxt(hWnd, id, buf, MAX_INPUT_LEN);
 if(1 != _stscanf(buf, _T("%lg %c"), &res, &dummy))
 {                              // garbage
  res = vdef;
 }
 if(vmax > vmin)
 {
  if(res < vmin)
   res = vmin;
  if(res > vmax)
   res = vmax;
 }

 TXT_PrintTxt(hWnd, id, prfmt? prfmt : _T("%.14G"), res);

 return res;
}

/* List box
 * ---- ---
 */
/* set horizontal scroll extent in pixels
*/
void LB_SetHscroll(HWND hWnd, int id, int ext_hsize)
{
 SendMessage(GetDlgItem(hWnd, id),  LB_SETHORIZONTALEXTENT,
                (WPARAM)(ext_hsize), (LPARAM)0);

}

/* print a line to list box
*/
void LB_Printf(HWND hWnd, int id, const TCHAR *fmt, ...)
{
 va_list va_params;
 int index;
 HWND hwndL;
 TCHAR *tmpmsg = cmalloc((MAX_PRINT_LEN + 1) * sizeof(TCHAR));  // enough??

 va_start(va_params, fmt);

 if(-1 == _vsntprintf(tmpmsg, MAX_PRINT_LEN, fmt, va_params))
  tmpmsg[MAX_PRINT_LEN] = _T('\0');

 hwndL = GetDlgItem(hWnd, id);
 if(hwndL)
 {
  while(LB_ERR == (index = ListBox_AddString(hwndL, tmpmsg)))
   ListBox_DeleteString(hwndL, 0);

  ListBox_SetCurSel(hwndL, index);
 }
 va_end(va_params);
 free(tmpmsg);
}

/* Clear list box
*/
void LB_ClearAll(HWND hWnd, int id)
{
 ListBox_ResetContent(GetDlgItem(hWnd, id));
}

/* set current selection of list
*/
int LB_SetSel(HWND hWnd, int id, int ix)
{
 return ListBox_SetCurSel(GetDlgItem(hWnd, id), ix);
}

/* get current list index and data
*/
int LB_GetIndexRec(HWND hWnd, int id, LRESULT *param)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix;

 ix = ListBox_GetCurSel(hwndL);
 if(param)
 {
  *param = ListBox_GetItemData(hwndL, ix);
 }
 return ix;
}

/* add to the end of list string with associated data
*/
int LB_AddRecord(HWND hWnd, int id, LPCTSTR str, LPARAM param)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix;

 ix = ListBox_AddString(hwndL, str);
 ListBox_SetItemData(hwndL, ix, param);
 ListBox_SetCurSel(hwndL, ix);
 return ix;
}

/* remove the last item from list
*/
int LB_DelRecord(HWND hWnd, int id)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix, need_move = 0;

 ix = ListBox_GetCount(hwndL);
 if(!ix)
  return LB_ERR;

 if(ListBox_GetCurSel(hwndL) == --ix)
  need_move = 1;

 ListBox_DeleteString(hwndL, ix);

 if(ix && need_move)
  ListBox_SetCurSel(hwndL, --ix);

 return ListBox_GetCurSel(hwndL);
}

/* Combo box
 * ----- ---
 */
/* Clear combo box
*/
void CB_ClearAll(HWND hWnd, int id)
{
 ComboBox_ResetContent(GetDlgItem(hWnd, id));
}

/* set current selection of combo box list
*/
int CB_SetSel(HWND hWnd, int id, int ix)
{
 return ComboBox_SetCurSel(GetDlgItem(hWnd, id), ix);
}

/* get current list index and data
*/
int CB_GetIndexRec(HWND hWnd, int id, LRESULT *param)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix;

 ix = ComboBox_GetCurSel(hwndL);
 if(param)
 {
  *param = ComboBox_GetItemData(hwndL, ix);
 }
 return ix;
}

/* add to the end of list string with associated data
*/
int CB_AddRecord(HWND hWnd, int id, LPCTSTR str, LPARAM param)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix;

 ix = ComboBox_AddString(hwndL, str);
 ComboBox_SetItemData(hwndL, ix, param);
 ComboBox_SetCurSel(hwndL, ix);
 return ix;
}

/* remove the last item from combo box list
*/
int CB_DelRecord(HWND hWnd, int id)
{
 HWND hwndL = GetDlgItem(hWnd, id);
 int ix, need_move = 0;

 ix = ComboBox_GetCount(hwndL);
 if(!ix)
  return LB_ERR;

 if(ComboBox_GetCurSel(hwndL) == --ix)
  need_move = 1;

 ComboBox_DeleteString(hwndL, ix);

 if(ix && need_move)
  ComboBox_SetCurSel(hwndL, --ix);

 return ComboBox_GetCurSel(hwndL);
}

/* the end...
*/

