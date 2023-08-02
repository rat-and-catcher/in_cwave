/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      compat_win32_gcc.h -- MinGW gcc compatibility -- "postinclude" part
 *      NOTE: this file declare Win32 API objects, which absent in MinGW gcc V5.1.0
 *      (supplied with Code::Blocks 17.12). It's not work with more recent versions
 *      of MinGw. Please take care about compatibility with your version by editing
 *      this file (and, probably, compat_gcc.h too). Sorry.
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

#if !defined(_compat_win32_gcc_h_)
#define _compat_win32_gcc_h_

// This is for gcc only
#if defined(__GNUC__)

// omited parts Win32 SDK

// @winbase.h::
WINBASEAPI BOOL WINAPI GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);

WINBASEAPI LONGLONG WINAPI InterlockedCompareExchange64(
    LONGLONG volatile *Destination,
    LONGLONG Exchange,
    LONGLONG Comperand
    );

// @tchar.h::
typedef wint_t _TINT;
#if defined(UNICODE)
#define _istblank   iswblank
#else
#define _istblank   isblank
#endif

// @ShlObj.h::
typedef enum {
    SHGFP_TYPE_CURRENT  = 0,   // current value for user, verify it exists
    SHGFP_TYPE_DEFAULT  = 1,   // default value, may not exist
} SHGFP_TYPE;

// @CommCtrl.h::
#define WINCOMMCTRLAPI  __declspec(dllimport)
typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;             // size of this structure
    DWORD dwICC;              // flags indicating which classes to be initialized
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;
#define ICC_INTERNET_CLASSES   0x00000800
#define ICC_PAGESCROLLER_CLASS 0x00001000   // page scroller
#define ICC_NATIVEFNTCTL_CLASS 0x00002000   // native font control
#define ICC_STANDARD_CLASSES   0x00004000
#define ICC_LINK_CLASS         0x00008000
WINCOMMCTRLAPI BOOL WINAPI InitCommonControlsEx(const INITCOMMONCONTROLSEX *picce);

// @MMReg.h::
#if !defined(DEFINE_WAVEFORMATEX_GUID)
#define DEFINE_WAVEFORMATEX_GUID(x)     \
        (USHORT)(x), 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
#endif

#if !defined(STATIC_KSDATAFORMAT_SUBTYPE_PCM)
#define STATIC_KSDATAFORMAT_SUBTYPE_PCM \
        DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_PCM)
#endif

#if !defined(STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
#define STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT \
        DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_IEEE_FLOAT)
#endif

#endif          // def __GNUC__

#endif          // def _compat_win32_gcc_h_

/* the end...
*/

