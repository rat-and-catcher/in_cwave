/*
 * mt_jrnd.c -- the random number generator module implementation
 * source -- mt19937ar.c (2002/1/26; 2002/2/10)
 * Original distribution file name -- mt19937ar.sep.tgz
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 *
 * One of implementations of the algorithm now (2016) is a part of C++ STL library.
 *
 * Adaptation for internal needs and minor fixes by Rat and Catcher Technologies.
 * (c) 2016-2020 Rat and Catcher Technologies. All Rights Reserved. We don't
 * apply any copyright limitation to the code of this file.
 */

#include "mt_jrnd.h"

// local definitions
#define NN              (MT_JRND_NN)            /* period parameter(s) */
#define MM              (397)
#define MATRIX_A        (0x9908B0DFU)           /* constant vector a */
#define UMASK           (0x80000000U)           /* most significant w-r bits */
#define LMASK           (0x7FFFFFFFU)           /* least significant r bits */
#define MIXBITS(ux, vx) (((ux) & UMASK) | ((vx) & LMASK))
#define TWIST(ux, vx)   ((MIXBITS(ux, vx) >> 1) ^ ((vx) & 1U ? MATRIX_A : 0U))

/* initialize the state with (from) a seed
*/
MT_JRND_STATE *mtrnd_init_seed(MT_JRND_STATE *self, uint32_t seed)
{
 uint32_t j;

 self -> state[0] = seed & 0xFFFFFFFFU;         // for >32 bit machines (:-/)

 for(j = 1; j < NN; j++)
 {
  self -> state[j] = (1812433253U * (self -> state[j - 1] ^ (self -> state[j - 1] >> 30)) + j);
  // See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
  // In the previous versions, MSBs of the seed affect
  // only MSBs of the array state[].
  // 2002/01/09 modified by Makoto Matsumoto

  self -> state[j] &= 0xFFFFFFFFU;              // for >32 bit machines (:-/)
 }

 self -> left = 1;
 return self;
}

/* initialization with array 'init_key' with length 'key_length'
*/
MT_JRND_STATE *mtrnd_init_key(MT_JRND_STATE *self, const uint32_t init_key[], uint32_t key_length)
{
 uint32_t i, j, k;

 (void)mtrnd_init_seed(self, 19650218U);

 for (i = 1, j = 0, k = (NN > key_length ? NN : key_length); k; --k)
 {
  self -> state[i] =
        (self -> state[i] ^
        ((self -> state[i - 1] ^ (self -> state[i - 1] >> 30)) * 1664525U)) +
        init_key[j] + j;                // non linear

  self -> state[i] &= 0xFFFFFFFFU;      // for WORDSIZE > 32 machines (:-/)

  if(++i >= NN)
  {
   self -> state[0] = self -> state[NN - 1];
   i = 1;
  }
  if (++j >= key_length)
  {
   j = 0;
  }
 }

 for (k = NN - 1; k; --k)
 {
  self -> state[i] =
        (self -> state[i] ^
        ((self -> state[i - 1] ^ (self -> state[i - 1] >> 30)) * 1566083941U)) - i;     // non linear

  self -> state[i] &= 0xFFFFFFFFU;      // for WORDSIZE > 32 machines (:-/)

  if(++i >= NN)
  {
   self -> state[0] = self -> state[NN - 1];
   i = 1;
  }
 }

 self -> state[0] = 0x80000000U;        // MSB is 1; assuring non-zero initial array
 self -> left = 1;
 return self;
}

/* VERY BASIC -- generate next uint32-random number
*/
uint32_t mtrnd_gen_ui32(MT_JRND_STATE *self)
{
 uint32_t y;

 if(0 == --(self -> left))              // need to create the next state
 {
  // the next state creation; as-is))
  uint32_t *p = self -> state;
  unsigned j;

  self -> left = NN;
  self -> next = self -> state;         // here and only here .next obtain it's value

  for (j = NN - MM + 1; --j; ++p)
  {
   *p = p[MM] ^ TWIST(p[0], p[1]);
  }

  for (j = MM; --j; ++p)
  {
   *p = *((p + MM) - NN) ^ TWIST(p[0], p[1]);
  }

  *p = *((p + MM) - NN) ^ TWIST(p[0], self -> state[0]);
 }

 y = *(self -> next)++;                 // here and only here .next uses

 // Tempering
 y ^= (y >> 11);
 y ^= (y <<  7) & 0x9D2C5680U;
 y ^= (y << 15) & 0xEFC60000U;
 y ^= (y >> 18);

 return y;
}

/* generate -maxint..maxint number
*/
int32_t mtrnd_gen_si32(MT_JRND_STATE *self)
{
 return (int32_t)mtrnd_gen_ui32(self);
}

/* generate 0..maxint (signed) number
*/
int32_t mtrnd_gen_hi32(MT_JRND_STATE *self)
{
 return (int32_t)(mtrnd_gen_ui32(self) >> 1);
}

/* generate full scale 64-bits unsigned random number ([0,0xFFFFFFFFFFFFFFFF]-interval)
*/
uint64_t mtrnd_gen_ui64(MT_JRND_STATE *self)
{
 uint64_t lo = mtrnd_gen_ui32(self);
 uint64_t hi = mtrnd_gen_ui32(self);
 return lo | (hi << 32);
}

/* generate -maxlonglong..maxlonglong number
*/
int64_t mtrnd_gen_si64(MT_JRND_STATE *self)
{
 return (int64_t)mtrnd_gen_ui64(self);
}

/* generate 0..maxlonglong (signed) number
*/
int64_t mtrnd_gen_hi64(MT_JRND_STATE *self)
{
 return (int64_t)(mtrnd_gen_ui64(self) >> 1);
}

/* LOWCOST::generates a random number on [0,1]-real-interval
*/
double mtrnd_gen_dlclosed(MT_JRND_STATE *self)
{
 return ((double)mtrnd_gen_ui32(self)) * (1.0 / 4294967295.0);
 // divided by 2^32-1
}

/* LOWCOST::generates a random number on [0,1)-real-interval
*/
double mtrnd_gen_dlsemi(MT_JRND_STATE *self)
{
 return ((double)mtrnd_gen_ui32(self)) * (1.0 / 4294967296.0);
 // divided by 2^32
}

/* LOWCOST::generates a random number on (0,1)-real-interval
*/
double mtrnd_gen_dlopen(MT_JRND_STATE *self)
{
 double res;

 do
 {
  res = mtrnd_gen_dlsemi(self);
 }
 while(0.0 == res);

 return res;
}

/* generate a random number on [0,1] with 53-bit resolution
*/
double mtrnd_gen_dclosed(MT_JRND_STATE *self)
{
 // this is, probably, WRONG.
 uint32_t a = mtrnd_gen_ui32(self) >> 5;
 uint32_t b = mtrnd_gen_ui32(self) >> 6;
 // a::[0..2**27-1]; b::[0..2**26-1]
 //    2**26__________          2**53__________________
 return(a * 67108864.0 + b) * (1.0 / 9007199254740991.0);
}

/* generate a random number on [0,1) with 53-bit resolution
*/
double mtrnd_gen_dsemi(MT_JRND_STATE *self)
{
 // this is, probably, WRONG.
 uint32_t a = mtrnd_gen_ui32(self) >> 5;
 uint32_t b = mtrnd_gen_ui32(self) >> 6;
 // a::[0..2**27-1]; b::[0..2**26-1]
 //    2**26__________          2**53__________________
 return(a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}

/* generate a random number on (0,1) with 53-bit resolution
*/
double mtrnd_gen_dopen(MT_JRND_STATE *self)
{
 double res;

 do
 {
  res = mtrnd_gen_dsemi(self);
 }
 while(0.0 == res);

 return res;
}

/* generate a random number on (-1,1) with 53-bit resolution
*/
double mtrnd_gen_dsopen(MT_JRND_STATE *self)
{
 double res;

 do
 {
  res = mtrnd_gen_dsemi(self) * 2.0 - 1.0;
 }
 while(-1.0 == res || 1.0 == res);

 return res;
}


 /* the end...
*/

