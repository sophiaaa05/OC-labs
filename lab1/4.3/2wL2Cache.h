#ifndef L2CACHE_H
#define L2CACHE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../base_code/Cache.h"

#define L1_LINES L1_SIZE / BLOCK_SIZE
#define L2_LINES L2_SIZE / BLOCK_SIZE

#define L2_WAYS 2

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/



/*********************** Cache *************************/

void initCache();

typedef enum {
  L1_CACHE,
  L2_CACHE,
} cache_type;

typedef struct cache_line {
  uint32_t tag;
  bool valid;
  bool dirty;
  uint32_t lru;
} cache_line;

typedef struct cache {
  uint32_t init;
  cache_line lines[L2_LINES];
} cache;


/*********************** Interfaces *************************/

void read(uint32_t, uint8_t*);

void write(uint32_t, uint8_t*);

#endif