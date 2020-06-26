/*
 *      CWAVE (RWAVE==WAV) input plugin / quad modulator for WinAmp (XMPlay) player
 *
 *      xwave_reader.c -- all about reading CWAVE and WAV ('RWAVE')
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

/* NOTE: we assume that there is only 1 or 2 channels; >2 channels not supported
*/

#include "in_cwave.h"
#include "unpack_lsb.h"

/* THE WAV FILE ANATOMY -- AS WE UNDERSTAND
 * --- --- ---- ------- -- -- -- ----------
 */
// 1. RIFF header:
// "RIFF" -- 4 bytes
// <full file length> - 8 bytes (all of the file after the field) -- 4 bytes (_may contain a garbage_)
// "WAVE" -- 4 bytes
// 2+. <set of the chuknks> with some data. Each chunk contain a header and a body.
// The chunk header has the next format:
// 2.1 <chunk ID> -- 4 bytes, usally 4 ASCII letters;
// 2.2 <body chunk size in bytes> -- 4 bytes; size of chunk body.
// The WAV file MUST contain _one_ chunk with the ID "fmt ", which describe format of
// audiodata, and _one_ chunk "data", which contain audiodata according "fmt ".
// The "fmt " chunk must present _before_ the data chunk. All of the other
// chunks (if any) can contain some well-known (like ID3-tag) or application-specific
// metadata; we ignore them. Data may be encoding as RAW integer or IEEE-754 float
// raw data, and we will try to handle it; as well as data may be compressed
// whith some ways -- we don't handle it.
// So, _body_ of the "fmt " chunk can be:
// -- Deprecated WAVEFORMAT structure (we try to read this, but never to write):
// typedef struct {
//  WORD  wFormatTag;                   // +0
//  WORD  nChannels;                    // +2
//  DWORD nSamplesPerSec;               // +4
//  DWORD nAvgBytesPerSec;              // +8
//  WORD  nBlockAlign;                  // +12
// } WAVEFORMAT;                        // sizeof() == 14
// here interesting wformatTag, it can be WAVE_FORMAT_PCM only for THIS header type
// (see below WAV_FMT_TAG). Usually, if WAVE_FORMAT_PCM set, the header is:
// typedef struct {
//  WAVEFORMAT wf;                      // +0
//  WORD       wBitsPerSample;          // +14
// } PCMWAVEFORMAT;                     // sizeof == 16
// !!!!!!!!
// MOREOVER::wformatTag can be WAVE_FORMAT_IEEE_FLOAT, and PCMWAVEFORMAT will be VALID!!
// MOREOVER::lots of software, wich work with float WAVs misunderstood GUID'ed formats for
// floats; only PCMWAVEFORMAT with wBitsPerSample=32.
// !!!!!!!!
// some people say, that it's a bad practics to describe 24/32 bits files wusing PCMWAVEFORMAT,
// for this cases serve WAVEFORMATEX / WAVEFORMATEXTENSIBLE format descriptors:
// typedef struct {
//  WORD  wFormatTag;                   // +0
//  WORD  nChannels;                    // +2
//  DWORD nSamplesPerSec;               // +4
//  DWORD nAvgBytesPerSec;              // +8
//  WORD  nBlockAlign;                  // +12
//  WORD  wBitsPerSample;               // +14
//  WORD  cbSize;                       // +16 size of extra bytes
// } WAVEFORMATEX;                      // sizeof() = 18
// the new gay -- cbSize, can be ignored for 1-2 channels PCM 8-16 bits. And for 24-32 bits too.
// But good choise to use with 24+ bits and/or floats and/or for more than 2 channels
// WAVEFORMATEXTENSIBLE structure:
// typedef struct {
//  WAVEFORMATEX Format;                // +0
//  union {                             // +18
//      WORD wValidBitsPerSample;       // for PCM mean (reading) right samples shift to
//                                      // shift = bps - wValidBitsPerSample
//                                      // (LSBits of the sample are not valid)
//      WORD wSamplesPerBlock;
//      WORD wReserved;
//      } Samples;                      // +18
//  DWORD        dwChannelMask;         // +20 see below
//  GUID         SubFormat;             // +24, 16 bytes, see below
// } WAVEFORMATEXTENSIBLE;              // sizeof() == 40
// So, we accept format descriptors with 14, 16, 18, or 40 bytes length, with
// wFormatTag == WAVE_FORMAT_PCM or WAVE_FORMAT_IEEE_FLOAT, with sample size 8, 16, 24, 32 bits;
// OR wFormatTag == WAVE_FORMAT_EXTENSIBLE with GUID's:
// KSDATAFORMAT_SUBTYPE_PCM "00000001-0000-0010-8000-00aa00389b71"
// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT "00000003-0000-0010-8000-00aa00389b71"
// We assume, that all of the floats has normalized to 1.0.
// Channel mask contain exactly nCannels set 1-value bits, so we can handle max. 32 channels.
// On write, we has began from LSBit (0x1 for one channel, 0x3 for two, 0x7 for three etc).
// NOTE::writing WAV files not supported in this version

/* the heplers
 * --- -------
 */
/* check file extension for our types
*/
static __inline unsigned check_file_ext(const TCHAR *pext)
{
 if(!_tcsicmp(pext, _T("CWAVE")))
  return XW_TYPE_CWAVE;

 if(!_tcsicmp(pext, _T("WAV")) || !_tcsicmp(pext, _T("RWAVE")))
  return XW_TYPE_RWAVE;

 return XW_TYPE_UNKNOWN;
}

/* get file size
*/
static __int64 get_file_size(XWAVE_READER *xr)
{
 LARGE_INTEGER li;
 BOOL res = GetFileSizeEx(xr -> file_hanle, &li);

 return res? li.QuadPart : 0;
}

/* read the fixed portion of the file
*/
static __inline size_t read_data(void *buf, size_t size, XWAVE_READER *xr)
{
 DWORD returned;
 BOOL res = ReadFile(xr -> file_hanle, buf, size, &returned, NULL);

 return res? ((size_t)returned) : 0;
}

/* internal "lite" seek for file header reading
*/
static __inline BOOL seek_bytes(__int64 offset, DWORD method, XWAVE_READER *xr)
{
 LARGE_INTEGER dist;

 dist.QuadPart = offset;
 return SetFilePointerEx(xr -> file_hanle, dist, NULL, method);
 // don't fix an error, if file pointer is "out of file length" -- we fix an
 // eeror condition in this case during read_data operation
}

/* unpackers (UNPACK_IQ) for supported types of CWAVE data
*/
// -- HCW_FMT_PCM_DBL64 (0)
static void unpack_cwave_dbl(double *vI, double *vQ, const BYTE **buf)
{
 *vI = unpack_double(*buf);
 *vQ = unpack_double(*buf + sizeof(double));
 *buf += sizeof(double) * 2;
}

// -- HCW_FMT_PCM_INT16 (1)
static void unpack_cwave_sht(double *vI, double *vQ, const BYTE **buf)
{
 *vI = (double)unpack_short(*buf);
 *vQ = (double)unpack_short(*buf + sizeof(short));
 *buf += sizeof(short) * 2;
}

// -- HCW_FMT_PCM_INT16_FLT32 (2)
static void unpack_cwave_sht_flt(double *vI, double *vQ, const BYTE **buf)
{
 *vI = (double)unpack_short(*buf);
 *vQ = (double)unpack_float(*buf + sizeof(short));
 *buf += sizeof(short) + sizeof(float);
}

// -- HCW_FMT_PCM_FLT32 (3)
static void unpack_cwave_flt(double *vI, double *vQ, const BYTE **buf)
{
 *vI = (double)unpack_float(*buf);
 *vQ = (double)unpack_float(*buf + sizeof(float));
 *buf += sizeof(float) * 2;
}

/* unpackers (UNPACK_RE) for supported types of RWAVE/WAV data
*/
// -- HRW_FMT_UINT8 (0)
static void unpack_rwave_uch(double *vR, const BYTE **buf)
{
 unsigned char *bptr = (unsigned char *)(*buf);

 *vR = 256.0 * (double)((signed char)(bptr[0] - 0x80U));
 *buf += sizeof(unsigned char);
}

// -- HRW_FMT_INT16 (1)
static void unpack_rwave_sht(double *vR, const BYTE **buf)
{
 *vR = (double)unpack_short(*buf);
 *buf += sizeof(short);
}

// -- HRW_FMT_INT24 (2)
static void unpack_rwave_i24(double *vR, const BYTE **buf)
{
 *vR = ((double)unpack_int24(*buf)) / 256.0;
 *buf += sizeof(unsigned char) * 3;
}

// -- HRW_FMT_INT32 (3)
static void unpack_rwave_int(double *vR, const BYTE **buf)
{
 *vR = ((double)unpack_int32(*buf)) / 65536.0;
 *buf += sizeof(int);
}

// -- HRW_FMT_FLOAT32 (4)
static void unpack_rwave_flt(double *vR, const BYTE **buf)
{
 *vR = 32768.0 * (double)unpack_float(*buf);    // probably *32767.0 looks better, but we prefer 2**N
 *buf += sizeof(float);
}

/* complete opening of cwave file (OK, it's write-only code)
*/
static BOOL cwave_reader_create(XWAVE_READER *xr)       // FALSE == BAD
{
 // supported one channel sample lengths for CWAVE
 static const unsigned cw_slen[] =
 {
  sizeof(double) * 2,                                   // HCW_FMT_PCM_DBL64 (0)
  sizeof(short) * 2,                                    // HCW_FMT_PCM_INT16 (1)
  sizeof(short) + sizeof(float),                        // HCW_FMT_PCM_INT16_FLT32 (2)
  sizeof(float) * 2                                     // HCW_FMT_PCM_FLT32 (3)
 };
 // known part of the header
 unsigned char hbuf[8 + sizeof(unsigned) * 7 + sizeof(int) + sizeof(double)];
 const size_t hsize = 8 + sizeof(unsigned) * 7 + sizeof(int) + sizeof(double);
 int i = 0;

 __int64 fsize = get_file_size(xr);

 if(fsize < hsize
        || read_data(hbuf, hsize, xr) != hsize
// .magic
        || (memcpy(&xr -> spec.cwave.header.magic, &hbuf[i], 8)
         , i += 8
         , memcmp(xr -> spec.cwave.header.magic, HCW_MAGIC, 8))
// .hsize
        || (memcpy(&xr -> spec.cwave.header.hsize, &hbuf[i], sizeof(unsigned))
         , i += sizeof(unsigned)
         , xr -> spec.cwave.header.hsize < hsize)
        || xr -> spec.cwave.header.hsize >= fsize
// .version
        || (memcpy(&xr -> spec.cwave.header.version, &hbuf[i], sizeof(unsigned))
         , i += sizeof(unsigned)
         , (xr -> spec.cwave.header.version != HCW_VERSION_V1
                && xr -> spec.cwave.header.version != HCW_VERSION_V2))
// .format
        || (memcpy(&xr -> spec.cwave.header.format, &hbuf[i], sizeof(unsigned))
         , i += sizeof(unsigned)
         , xr -> spec.cwave.header.format > HCW_FMT_PCM_FLT32)
// .n_channels
        || (memcpy(&xr -> spec.cwave.header.n_channels, &hbuf[i], sizeof(unsigned))
         , i += sizeof(unsigned)
         , xr -> spec.cwave.header.n_channels > 2)
        || xr -> spec.cwave.header.n_channels == 0
// .n_samples
        || (memcpy(&xr -> spec.cwave.header.n_samples, &hbuf[i], sizeof(unsigned))
         , i += sizeof(unsigned)
         , xr -> spec.cwave.header.n_samples < MIN_FILE_SAMPLES)
        || xr -> spec.cwave.header.n_samples
                        * xr -> spec.cwave.header.n_channels
                        * cw_slen[xr -> spec.cwave.header.format]
                        + xr -> spec.cwave.header.hsize > fsize
// .sample_rate + unchecked fields (.k_M, .n_CRC32, .k_beta)
        || (memcpy(&xr -> spec.cwave.header.sample_rate, &hbuf[i], sizeof(unsigned)), i += sizeof(unsigned)
         , memcpy(&xr -> spec.cwave.header.k_M, &hbuf[i], sizeof(int)), i += sizeof(int)
         , memcpy(&xr -> spec.cwave.header.n_CRC32, &hbuf[i], sizeof(unsigned)), i += sizeof(unsigned)
         , memcpy(&xr -> spec.cwave.header.k_beta, &hbuf[i], sizeof(double))
         , xr -> spec.cwave.header.sample_rate == 0))
  return FALSE;

 // fill the rest
 crc32init(&xr -> spec.cwave.cur_crc);                  // just in case
 xr -> n_channels       = xr -> spec.cwave.header.n_channels;
 xr -> n_samples        = xr -> spec.cwave.header.n_samples;
 xr -> sample_rate      = xr -> spec.cwave.header.sample_rate;
 xr -> sample_size      = xr -> spec.cwave.header.n_channels
                        * cw_slen[xr -> spec.cwave.header.format];
 xr -> offset_data      = xr -> spec.cwave.header.hsize;

 switch(xr -> spec.cwave.header.format)
 {
  case HCW_FMT_PCM_DBL64:
   xr -> unpack_handler.unpack_iq = &unpack_cwave_dbl;
   break;
  case HCW_FMT_PCM_INT16:
   xr -> unpack_handler.unpack_iq = &unpack_cwave_sht;
   break;
  case HCW_FMT_PCM_INT16_FLT32:
   xr -> unpack_handler.unpack_iq = &unpack_cwave_sht_flt;
   break;
  case HCW_FMT_PCM_FLT32:
   xr -> unpack_handler.unpack_iq = &unpack_cwave_flt;
   break;
 }

 return xwave_seek_samples(0, xr);
}

/* helper - convert WAV's PCM (int only) BPS to HRW_FMT_xxx (excl. HRW_FMT_FLOAT32)
*/
static unsigned pcmBpsToFormat(unsigned bps)
{
 switch(bps)
 {
  case 8:                                       // 8 bits
   return HRW_FMT_UINT8;
  case 16:                                      // 16 bits
   return HRW_FMT_INT16;
  case 24:                                      // 24 bits
   return HRW_FMT_INT24;
  case 32:                                      // 32 bits
   return HRW_FMT_INT32;
  default:                                      // inacceptable
   break;
 }
 return HRW_FMT_UNKNOWN;                        // error
}
/* complete opening of rwave (WAV) file (OK, it's completely write-only code)
*/
static BOOL rwave_reader_create(XWAVE_READER *xr)       // FALSE == BAD
{
 // supported GUIDs
 static const GUID guidPcm = { STATIC_KSDATAFORMAT_SUBTYPE_PCM };
 static const GUID guidFloat = { STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT };

 // some header sizes
 static const size_t sizWAVEFORMAT = sizeof(WORD) * 3 + sizeof(DWORD) * 2;
 static const size_t sizPCMWAVEFORMAT = sizeof(WORD) * 4 + sizeof(DWORD) * 2;
 static const size_t sizWAVEFORMATEXTENSIBLE = sizeof(WORD) * 6 + sizeof(DWORD) * 3 + sizeof(GUID);
 // and correspondig buffer with max possible size -- size, wich we can eat
 unsigned char fmtbuf[sizeof(WORD) * 6 + sizeof(DWORD) * 3 + sizeof(GUID)];

 __int64 fsize = get_file_size(xr);                     // don't check here
 __int64 fpos = 0;
 unsigned fmt_ch_len, data_ch_len;

 unsigned char hchunk[8];                               // chunk header FOURCC(4)<body_len>(4)

 {      // first of all, we read file header "RIFF(4)<filelen-8>(4)WAVE(4)". <filelen> will be ignored.
  unsigned char hbuf[12];

  if(read_data(hbuf, 12, xr) != 12
        || memcmp(&hbuf[0], "RIFF", 4)
        || memcmp(&hbuf[8], "WAVE", 4))
   return FALSE;
  fpos += 12;
 }

 // next, we must to find "fmt " chunk. If we found "data" chunk before -- error
 for(;;)
 {
  if(read_data(hchunk, 8, xr) != 8)
   return FALSE;                                        // on EOF die here
  memcpy(&fmt_ch_len, &hchunk[4], 4);
  fpos += 8;
  if(fpos + fmt_ch_len > fsize)                         // or die here on EOF
   return FALSE;

  if(!memcmp(hchunk, "fmt ", 4))
   break;                                               // ok - found

  if(!memcmp(hchunk, "data", 4))
   return FALSE;                                        // bad - "data" w/o (or before) "fmt "

  if(!seek_bytes(fmt_ch_len, FILE_CURRENT, xr))         // move forward
   return FALSE;
  fpos += fmt_ch_len;
 }

 if(fmt_ch_len < sizWAVEFORMAT)                         // normal people say here sizPCMWAVEFORMAT
  return FALSE;                                         // but we had met files with WAVEFORMAT only

 // "fmt " found. try to read and parse it
 {
  const size_t fmt_ch_len_toread = fmt_ch_len < sizWAVEFORMATEXTENSIBLE?
        fmt_ch_len
        :
        sizWAVEFORMATEXTENSIBLE;
  int i = 0;

  if(read_data(fmtbuf, fmt_ch_len_toread, xr) != fmt_ch_len_toread)
   return FALSE;

  if(fmt_ch_len_toread < fmt_ch_len)
  {                                                     // skip the rest, if any
   if(!seek_bytes(fmt_ch_len - fmt_ch_len_toread, FILE_CURRENT, xr))
    return FALSE;
  }
  fpos += fmt_ch_len;                                   // we don't cross EOF here

  // ++ WAVEFORMAT, all fields must be valid
  memcpy(&xr -> spec.rwave.header.Format.wFormatTag, &fmtbuf[i], sizeof(WORD)), i += sizeof(WORD);
  memcpy(&xr -> spec.rwave.header.Format.nChannels, &fmtbuf[i], sizeof(WORD)), i += sizeof(WORD);
  memcpy(&xr -> spec.rwave.header.Format.nSamplesPerSec, &fmtbuf[i], sizeof(DWORD)), i += sizeof(DWORD);
  memcpy(&xr -> spec.rwave.header.Format.nAvgBytesPerSec, &fmtbuf[i], sizeof(DWORD)), i += sizeof(DWORD);
  memcpy(&xr -> spec.rwave.header.Format.nBlockAlign, &fmtbuf[i], sizeof(WORD)), i += sizeof(WORD);
  // -- WAVEFORMAT
  // ++ PCMWAVEFORMAT, probably garbage
  memcpy(&xr -> spec.rwave.header.Format.wBitsPerSample, &fmtbuf[i], sizeof(WORD)), i += sizeof(WORD);
  // -- PCMWAVEFORMAT, probably garbage

  // just in case (paranoja): round m_bps up to m_bps % 8 == 0
  xr -> spec.rwave.header.Format.wBitsPerSample =
        (xr -> spec.rwave.header.Format.wBitsPerSample + 7) & (~07);

  if(!xr -> spec.rwave.header.Format.nChannels
        || xr -> spec.rwave.header.Format.nChannels > 2
        || !xr -> spec.rwave.header.Format.nSamplesPerSec)
   return FALSE;

  // determinate xr -> spec.rwave.htype and spec.rwave.format. And make some checks.
  switch(xr -> spec.rwave.header.Format.wFormatTag)
  {
   case WAVE_FORMAT_PCM:
    if(fmt_ch_len < sizPCMWAVEFORMAT)
    {
     // SILLY&STUPID::this is impossible; this "format" notexistant. We know about.
     // take a risk to make some calculations
     if(xr -> spec.rwave.header.Format.nBlockAlign % xr -> spec.rwave.header.Format.nChannels)
      return FALSE;                     // can't determinate sample type
     // bps here and only here had wrong value
     xr -> spec.rwave.header.Format.wBitsPerSample =
        (xr -> spec.rwave.header.Format.nBlockAlign / xr -> spec.rwave.header.Format.nChannels) << 3;
     xr -> spec.rwave.htype = HRW_HTYPE_WFONLY;
    }
    else                                // usual way - ordinary PCMWAVEFORMAT
     xr -> spec.rwave.htype = HRW_HTYPE_PCMW;

    if((xr -> spec.rwave.format =
        pcmBpsToFormat(xr -> spec.rwave.header.Format.wBitsPerSample)) == HRW_FMT_UNKNOWN)
     return FALSE;

    break;

   case WAVE_FORMAT_IEEE_FLOAT:         // same as PCMWAVEFORMAT
    if(fmt_ch_len < sizPCMWAVEFORMAT
        || xr -> spec.rwave.header.Format.wBitsPerSample != 32)
     return FALSE;
    xr -> spec.rwave.htype = HRW_HTYPE_PCMW;
    xr -> spec.rwave.format = HRW_FMT_FLOAT32;
    break;

   case WAVE_FORMAT_EXTENSIBLE:
    // here and only here we can to get non-zero m_wavShift
    if(fmt_ch_len < sizWAVEFORMATEXTENSIBLE
        || (memcpy(&xr -> spec.rwave.header.Format.cbSize, &fmtbuf[i], sizeof(WORD))
         , i += sizeof(WORD)
         , xr -> spec.rwave.header.Format.cbSize < (sizeof(WORD) + sizeof(DWORD) + sizeof(GUID))))
     return FALSE;

    xr -> spec.rwave.htype = HRW_HTYPE_EXT;

    memcpy(&xr -> spec.rwave.header.Samples.wValidBitsPerSample, &fmtbuf[i], sizeof(WORD)), i += sizeof(WORD);
    memcpy(&xr -> spec.rwave.header.dwChannelMask, &fmtbuf[i], sizeof(DWORD)), i += sizeof(DWORD);
    memcpy(&xr -> spec.rwave.header.SubFormat, &fmtbuf[i], sizeof(GUID)); /* i dont need anymore */

    if(!memcmp(&xr -> spec.rwave.header.SubFormat, &guidPcm, sizeof(GUID)))
    {
     // PCM, almost same as "type 1" == WAVE_FORMAT_PCM
       if((xr -> spec.rwave.format =
        pcmBpsToFormat(xr -> spec.rwave.header.Format.wBitsPerSample)) == HRW_FMT_UNKNOWN)
      return FALSE;
     break;
    }

    if(!memcmp(&xr -> spec.rwave.header.SubFormat, &guidFloat, sizeof(GUID)))
    {
     // float, same as "type 3" == WAVE_FORMAT_IEEE_FLOAT
     if(xr -> spec.rwave.header.Format.wBitsPerSample != 32)
      return FALSE;
     xr -> spec.rwave.format = HRW_FMT_FLOAT32;
     break;
    }

    return FALSE;                               // unknown GUID
    break;

   default:
    return FALSE;
    break;
  }
  // OK, now we have a good Microsoft's .header with known .format and .htype.
 }

 // next, we must to find "data" chunk
 for(;;)
 {
  if(read_data(hchunk, 8, xr) != 8)
   return FALSE;                                        // on EOF die here
  fpos += 8;
  memcpy(&data_ch_len, &hchunk[4], 4);
  if(fpos + data_ch_len > fsize)                        // or die here on EOF
   return FALSE;

  if(!memcmp(hchunk, "data", 4))
   break;                                               // data found and it lay in file bounds

  if(!seek_bytes(data_ch_len, FILE_CURRENT, xr))        // move forward
   return FALSE;
  fpos += data_ch_len;
 }

 // fill the rest of the header
 xr -> sample_size = (xr -> spec.rwave.header.Format.wBitsPerSample >> 3) *
        xr -> spec.rwave.header.Format.nChannels;
 xr -> n_channels = xr -> spec.rwave.header.Format.nChannels;
 if((xr -> n_samples = data_ch_len / xr -> sample_size) < MIN_FILE_SAMPLES
        || xr -> spec.rwave.header.Format.nBlockAlign != xr -> sample_size)
  return FALSE;
 xr -> sample_rate = xr -> spec.rwave.header.Format.nSamplesPerSec;
 xr -> offset_data = fpos;

 switch(xr -> spec.rwave.format)
 {
  case HRW_FMT_UINT8:
   xr -> unpack_handler.unpack_re = &unpack_rwave_uch;
   break;
  case HRW_FMT_INT16:
   xr -> unpack_handler.unpack_re = &unpack_rwave_sht;
   break;
  case HRW_FMT_INT24:
   xr -> unpack_handler.unpack_re = &unpack_rwave_i24;
   break;
  case HRW_FMT_INT32:
   xr -> unpack_handler.unpack_re = &unpack_rwave_int;
   break;
  case HRW_FMT_FLOAT32:
   xr -> unpack_handler.unpack_re = &unpack_rwave_flt;
   break;
 }

 return TRUE;                                           // don't need any seeks
}


/* public interface
 * ------ ---------
 */
/* create reader -- one reader per file; return NULL if error
*/
XWAVE_READER *xwave_reader_create(const TCHAR *name, unsigned read_quant)
{
 XWAVE_READER *xr = NULL;
 unsigned namelen, pnamelen, filetype;
 const TCHAR *p;

 if(!name || !*name)
  return NULL;
 namelen = _tcslen(name);

 // find and check file extension
 for(p = name + namelen - 1; p > name && *p != _T('.'); --p)
  /* nothing */;
 if(*p !=  _T('.'))
  return NULL;
 if(XW_TYPE_UNKNOWN == (filetype = check_file_ext(p + 1)))
  return NULL;

 // begin to construct the object
 xr = cmalloc(sizeof(XWAVE_READER));
 memset(xr, 0, sizeof(XWAVE_READER));           // dont care about NULL'ed init
 xr -> file_hanle = INVALID_HANDLE_VALUE;
 xr -> read_quant = read_quant;
 xr -> really_readed =
 xr -> unpacked = 0;
 xr -> type = filetype;
 _tcscpy(xr -> file_full_name = cmalloc((namelen + 1) * sizeof(TCHAR)), name);

 // parse name to path and pure name
 while(p > name && *p != _T('\\'))
  --p;
 pnamelen = _tcslen(p);
 if(*p == _T('\\'))                             // there is both name and path
 {
  size_t pathlen = (p - name) + 1;              // TCHARs
  memcpy(xr -> file_path = cmalloc((pathlen + 1) * sizeof(TCHAR))
        , name
        , pathlen * sizeof(TCHAR));
  xr -> file_path[pathlen] = _T('\0');
  _tcscpy(xr -> file_pure_name = cmalloc(pnamelen * sizeof(TCHAR)), p + 1);
 }
 else                                           // there is name and empty path
 {
  (xr -> file_path = cmalloc(sizeof(TCHAR)))[0] = _T('\0');
  // p == name here
  _tcscpy(xr -> file_pure_name = cmalloc((pnamelen + 1) * sizeof(TCHAR)), p);
 }

 // open a file
 if(INVALID_HANDLE_VALUE == (xr -> file_hanle = CreateFile(name,
        GENERIC_READ,                           // desired access
        FILE_SHARE_READ,                        // share mode
        NULL,                                   // security attributes
        OPEN_EXISTING,                          // creation disposition
        FILE_ATTRIBUTE_NORMAL,                  // flags and attributes
        NULL)))                                 // template file
  return xwave_reader_destroy(xr), NULL;

 // read the header and fill the rest of the object
 switch(xr -> type)
 {
  case XW_TYPE_CWAVE:
   xr -> is_sample_complex = TRUE;
   if(cwave_reader_create(xr))
    break;                                      // ok cwave
   return xwave_reader_destroy(xr), NULL;

  case XW_TYPE_RWAVE:
   xr -> is_sample_complex = FALSE;
   if(rwave_reader_create(xr))
    break;                                      // ok rwave
   return xwave_reader_destroy(xr), NULL;

  default:
   return xwave_reader_destroy(xr), NULL;
 }

 // @V2.1.0+ -- additional check sample rate
 if(xr -> sample_rate > MAX_FS_SRC)
  return xwave_reader_destroy(xr), NULL;

 // OK
 xr -> tbuff =
 xr -> ptr_tbuff =
        xr -> read_quant? cmalloc(xr -> sample_size * xr -> read_quant) : NULL;
 xr -> pos_samples = 0;
 return xr;
}

/* destroy the reader
*/
void xwave_reader_destroy(XWAVE_READER *xr)
{
 // don't care about LPF_HILBERT_QUAD l_hq/r_hq -- it's app-wide objects
 if(xr)
 {
  if(xr -> file_hanle != INVALID_HANDLE_VALUE)
  {
   CloseHandle(xr -> file_hanle);
   xr -> file_hanle = INVALID_HANDLE_VALUE;
  }
  if(xr -> tbuff)
  {
   free(xr -> tbuff);
   xr -> ptr_tbuff = xr -> tbuff = NULL;
  }
  if(xr -> file_pure_name)
  {
   free(xr -> file_pure_name);
   xr -> file_pure_name = NULL;
  }
  if(xr -> file_path)
  {
   free(xr -> file_path);
   xr -> file_path = NULL;
  }
  if(xr -> file_full_name)
  {
   free(xr -> file_full_name);
   xr -> file_full_name = NULL;
  }

  free(xr);
 }
}

/* abs seek in samples terms in the file (non-unpacked data will be lost)
*/
BOOL xwave_seek_samples(__int64 sample_pos, XWAVE_READER *xr)   // FALSE == bad
{
 LARGE_INTEGER to_set, returned;
 BOOL res;

 // what the hell??
 if(sample_pos >= xr -> n_samples)
 {
  sample_pos = xr -> n_samples;
 }

 to_set.QuadPart = sample_pos * xr -> sample_size + xr -> offset_data;
 res = SetFilePointerEx(xr -> file_hanle, to_set, &returned, FILE_BEGIN);
 xr -> pos_samples = sample_pos;                        // correct or incorrect -- the question..
 xr -> really_readed = xr -> unpacked = 0;
 xr -> ptr_tbuff = xr -> tbuff;
 return (BOOL)(res && to_set.QuadPart == returned.QuadPart);
}

/* abs seek in miliseconds terms in the file (non-unpacked data will be lost)
*/
BOOL xwave_seek_ms(__int64 ms_pos, XWAVE_READER *xr)    // FALSE == bad
{
 __int64 s_pos = ms_pos * (__int64)xr -> sample_rate;
 return xwave_seek_samples(s_pos / 1000LL, xr);
}

/* change read quant (non-unpacked data will be lost)
*/
void xwave_change_read_quant(unsigned new_rq, XWAVE_READER *xr)
{
 if(new_rq > xr -> read_quant)                                  // recreate only if its really need
 {
  // not a realloc() -- we don't need a copy old data to new buffer
  if(xr -> tbuff)
   free(xr -> tbuff);
  xr -> tbuff = cmalloc(xr -> sample_size * new_rq);            // new_rq != 0 here
 }
 xr -> read_quant = new_rq;
 xr -> ptr_tbuff = xr -> tbuff;
 xr -> really_readed = xr -> unpacked = 0;
}

/* read the next portion (read quant) of data (tbuff[read_quant])
*/
BOOL xwave_read_samples(XWAVE_READER *xr)
{
 DWORD readed = 0;
 BOOL res = TRUE;               // EOF is not error
 unsigned to_read = xr -> pos_samples + xr -> read_quant <= xr -> n_samples?
        xr -> read_quant
        :
        (unsigned)(xr -> n_samples - xr -> pos_samples);

 if(!xr -> read_quant)
 {
  xr -> really_readed = xr -> unpacked = 0;
  return FALSE;                 // can't be readed, if read_quant == 0
 }

 if(to_read)
  res = ReadFile(xr -> file_hanle, xr -> tbuff, to_read * xr -> sample_size, &readed, NULL);

 xr -> really_readed = res? readed / xr -> sample_size : 0;
 xr -> pos_samples += xr -> really_readed;
 xr -> unpacked = 0;
 xr -> ptr_tbuff = xr -> tbuff;
 // if readed % xr -> sample_size != 0 -> we got hard unknown error
 return res && (readed % xr -> sample_size == 0);
}

/* unpack one sample as complex for left and right channels (need external lock for Hilb. conv.)
*/
void xwave_unpack_csample(double *lI, double *lQ, double *rI, double *rQ,
        MOD_CONTEXT *mc, XWAVE_READER *xr)
{
 const BYTE *bptr = xr -> ptr_tbuff;

 if(xr -> unpacked >= xr -> really_readed)
 {
  *lI = *lQ = *rI = *rQ = 0.0;
  return;
 }

 if(xr -> is_sample_complex)
 {
  (*(xr -> unpack_handler.unpack_iq))(lI, lQ, &bptr);

  if(xr -> n_channels > 1)
  {
   (*(xr -> unpack_handler.unpack_iq))(rI, rQ, &bptr);
  }
  else
  {
   *rI = *lI;
   *rQ = *lQ;
  }
 }
 else
 {
  double val;

  (*(xr -> unpack_handler.unpack_re))(&val, &bptr);
  if(mc && mc -> h_left)                                // just in case
   hq_rp_process(val, lI, lQ, mc -> h_left, &(mc -> fes_hilb_left));
  else
  {
   *lI = val;
   *lQ = 0.0;
  }

  if(xr -> n_channels > 1)
   (*(xr -> unpack_handler.unpack_re))(&val, &bptr);
  // let's right channel's filters play with us (and keep their state to the next track)
  if(mc && mc -> h_right)                               // just in case
   hq_rp_process(val, rI, rQ, mc -> h_right, &(mc -> fes_hilb_right));
  else
  {
   *rI = val;
   *rQ = 0.0;
  }
 }
 xr -> ptr_tbuff += xr -> sample_size;
 ++(xr -> unpacked);
}


/* the end...
*/

