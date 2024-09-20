#ifndef L2CACHE_H
#define L2CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../base_code/Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);


/*********************** Cache *************************/

void initCache();

void accessL1( uint32_t,  uint8_t *,  uint32_t);
void accessL2( uint32_t,  uint8_t *,  access_mode);

typedef struct cache_line {
  uint32_t tag;
  uint8_t data[BLOCK_SIZE];
  uint8_t valid;
  uint8_t dirty;

} cache_line;

typedef struct cache {
  uint32_t init;
  cache_line lines[L2_SIZE / BLOCK_SIZE];
} cache;


/*********************** Interfaces *************************/

void read(uint32_t, uint8_t*);

void write(uint32_t, uint8_t*);

#endif