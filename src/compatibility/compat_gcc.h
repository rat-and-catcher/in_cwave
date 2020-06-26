/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      compat_gcc.h -- MinGW gcc compatibility -- "preinclude" part
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

#if !defined(_compat_gcc_h_)
#define _compat_gcc_h_

// This is for gcc only
#if defined(__GNUC__)

/* WHAT-DO-WE-WANT FROM GCC
 * --------------- ---- ---
 */
// unicode (Win32 UTF-16) support
#define GCC_UNICODE

// 1st of all -- what versions of gcc we supportted
// for successfull compilation ourself code we need GCC >= 4.6
#if !((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && (defined(__i386__)))
#error "You need GCC (MinGW) V4.6+ x86 for compilation the code"
#endif

// 2nd -- unicode support
#if defined(GCC_UNICODE)

#if !defined(UNICODE)
#define UNICODE
#endif

#if !defined(_UNICODE)
#define _UNICODE
#endif

#endif          // def GCC_UNICODE

/* See also compat_win32_gcc.h for "postinclude" macros/types
 * --- ---- ------------------ --- ------------- ------------
 */

#endif          // def __GNUC__

#endif          // def _compat_gcc_h_

/* the end...
*/

