/*
 * mt_jrnd.h -- the random number generator module declarations
 * source -- mt19937ar.c (2002/1/26; 2002/2/10)
 * Original distribution file name -- mt19937ar.sep.tgz
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 *
 * One of implementations of the algorithm now (2016) is a part of C++ STL library.
 *
 * Adaptation for internal needs and minor fixes by Rat and Catcher Technologies.
 * (c) 2016 Rat and Catcher Technologies. All Rights Reserved. We don't
 * apply any copyright limitation to the code of this file.
 */

#if !defined(_mt_jrnd_h_)
#define _mt_jrnd_h_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

// Period parameter
#define MT_JRND_NN              (624)                   /* period parameter(s) */

// The state/context of the generator
typedef struct tagMT_JRND_STATE
{
 uint32_t state[MT_JRND_NN];                            // the state vector
 uint32_t *next;
 int left;
} MT_JRND_STATE;


/* Exported functions
 * -------- ---------
 */
/* initialize the state with (from) a seed
*/
MT_JRND_STATE *mtrnd_init_seed(MT_JRND_STATE *self, uint32_t seed);
/* initialization with array 'init_key' with length 'key_length'
*/
MT_JRND_STATE *mtrnd_init_key(MT_JRND_STATE *self, const uint32_t init_key[], uint32_t key_length);

/* VERY BASIC -- generate next uint32-random number
*/
uint32_t mtrnd_gen_ui32(MT_JRND_STATE *self);
/* generate -maxint..maxint number
*/
int32_t mtrnd_gen_si32(MT_JRND_STATE *self);
/* generate 0..maxint (signed) number
*/
int32_t mtrnd_gen_hi32(MT_JRND_STATE *self);
/* generate full scale 64-bits unsigned random number ([0,0xFFFFFFFFFFFFFFFF]-interval)
*/
uint64_t mtrnd_gen_ui64(MT_JRND_STATE *self);
/* generate -maxlonglong..maxlonglong number
*/
int64_t mtrnd_gen_si64(MT_JRND_STATE *self);
/* generate 0..maxlonglong (signed) number
*/
int64_t mtrnd_gen_hi64(MT_JRND_STATE *self);

/* LOWCOST::generates a random number on [0,1]-real-interval
*/
double mtrnd_gen_dlclosed(MT_JRND_STATE *self);
/* LOWCOST::generates a random number on [0,1)-real-interval
*/
double mtrnd_gen_dlsemi(MT_JRND_STATE *self);
/* LOWCOST::generates a random number on (0,1)-real-interval
*/
double mtrnd_gen_dlopen(MT_JRND_STATE *self);

/* generate a random number on [0,1] with 53-bit resolution
*/
double mtrnd_gen_dclosed(MT_JRND_STATE *self);
/* generate a random number on [0,1) with 53-bit resolution
*/
double mtrnd_gen_dsemi(MT_JRND_STATE *self);
/* generate a random number on (0,1) with 53-bit resolution
*/
double mtrnd_gen_dopen(MT_JRND_STATE *self);
/* generate a random number on (-1,1) with 53-bit resolution
*/
double mtrnd_gen_dsopen(MT_JRND_STATE *self);


#if defined(__cplusplus)
}
#endif

#endif          // def _mt_jrnd_h_

/* the end...
*/

