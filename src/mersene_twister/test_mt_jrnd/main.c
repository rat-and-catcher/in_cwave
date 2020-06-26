/*
 * The test for mt-jrnd based on original first 1000 uint32 numbers
 * source -- mt19937ar.c, mtTest.c (2002/1/26; 2002/2/10)
 * Original distribution file name -- mt19937ar.sep.tgz
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 *
 * One of implementations of the algorithm now (2016) is a part of C++ SLT library.
 *
 * Adaptation for internal needs and minor fixes by Rat and Catcher Technologies.
 * (c) 2016-2020 Rat and Catcher Technologies. All Rights Reserved. We don't
 * apply any copyright limitation to the code of this file.
 */

#include <stdio.h>
#include <stdlib.h>

#include "../mt_jrnd.h"

// ...from mt19937ar_out.c::
extern const uint32_t test_u32[];                       /* the test numbers */
extern const uint32_t test_init[], test_ilength;        /* the test parameters */
extern const uint32_t test_ntotal;                      /* total uint32 numbers to test */

static void pok(void)
{
 printf("OK\n");
}

int main(int argc, char **argv)
{
 MT_JRND_STATE st;
 uint32_t i;

 printf("-- Test mtrnd_gen_ui32(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  uint32_t my = mtrnd_gen_ui32(&st), jpn = test_u32[i];
  if(jpn != my)
  {
   printf("FAIL! %u <> %u\n", jpn, my); exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_si32(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  int32_t my = mtrnd_gen_si32(&st), jpn = (int32_t)test_u32[i];
  if(jpn != my)
  {
   printf("FAIL! %d <> %d\n", jpn, my); exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_hi32(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  int32_t my = mtrnd_gen_hi32(&st), jpn = (int32_t)(test_u32[i] >> 1);
  if(jpn != my)
  {
   printf("FAIL! %d <> %d\n", jpn, my); exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_ui64(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  uint64_t my = mtrnd_gen_ui64(&st);
  uint64_t lo = ((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU);
  uint64_t hi = ((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU);
  uint64_t jpn = lo | (hi << 32);

  if(jpn != my)
  {
   printf("FAIL! %16I64X <> %16I64X\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_si64(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  int64_t my = mtrnd_gen_si64(&st);
  uint64_t lo = ((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU);
  uint64_t hi = ((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU);
  int64_t jpn = (int64_t)(lo | (hi << 32));

  if(jpn != my)
  {
   printf("FAIL! %16I64X <> %16I64X\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_hi64(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  int64_t my = mtrnd_gen_hi64(&st);
  uint64_t lo = ((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU);
  uint64_t hi = ((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU);
  int64_t jpn = (int64_t)((lo | (hi << 32)) >> 1);

  if(jpn != my)
  {
   printf("FAIL! %16I64X <> %16I64X\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dlclosed(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  double my = mtrnd_gen_dlclosed(&st);
  double jpn = ((double)test_u32[i]) * (1.0 / 4294967295.0);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dlsemi(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  double my = mtrnd_gen_dlsemi(&st);
  double jpn = ((double)test_u32[i]) * (1.0 / 4294967296.0); ;

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dlopen(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; ++i)
 {
  double my;
  double jpn =  ((double)test_u32[i]) * (1.0 / 4294967296.0);

  if(0.0 == jpn)
   continue;

  my = mtrnd_gen_dlopen(&st);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dclosed(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  double my = mtrnd_gen_dclosed(&st);
  uint64_t a = (((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU)) >> 5;
  uint64_t b = (((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU)) >> 6;
  double jpn = (a * 67108864.0 + b) * (1.0 / 9007199254740991.0);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dsemi(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  double my = mtrnd_gen_dsemi(&st);
  uint64_t a = (((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU)) >> 5;
  uint64_t b = (((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU)) >> 6;
  double jpn = (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dopen(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  double my;
  uint64_t a = (((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU)) >> 5;
  uint64_t b = (((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU)) >> 6;
  double jpn = (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);

  if(0.0 == jpn)
   continue;

  my = mtrnd_gen_dopen(&st);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();

 printf("-- Test mtrnd_gen_dsopen(): "); fflush(stdout);
 (void)mtrnd_init_key(&st, test_init, test_ilength);
 for(i = 0; i < test_ntotal; i += 2)
 {
  double my;
  uint64_t a = (((uint64_t)test_u32[i    ]) & ((uint64_t)0xFFFFFFFFU)) >> 5;
  uint64_t b = (((uint64_t)test_u32[i + 1]) & ((uint64_t)0xFFFFFFFFU)) >> 6;
  double jpn = ((a * 67108864.0 + b) * (2.0 / 9007199254740992.0)) - 1.0;

  if(-1.0 == jpn || 1.0 == jpn)
   continue;

  my = mtrnd_gen_dsopen(&st);

  if(jpn != my)
  {
   printf("FAIL! %.16f <> %.16f\n", jpn, my);
   exit(1);
  }
 }
 pok();


 printf("-- EVERYTHING WENT OK\n");
 return 0;
}

/* the end...
*/

