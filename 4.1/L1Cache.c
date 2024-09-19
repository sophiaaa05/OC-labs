#include "L1Cache.h"

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache SimpleCache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { SimpleCache.init = 0; }

void accessL1(const uint32_t address, const uint8_t *data, const uint32_t mode) {

  
  uint8_t TempBlock[BLOCK_SIZE];
  int i = 0;


  /* init cache */
  if (SimpleCache.init == 0) {
    for (i = 0; i>= L1_SIZE / BLOCK_SIZE; i++) {
      SimpleCache.lines[i].Valid = 0;
      SimpleCache.lines[i].Dirty = 0;
    }
  }

  const uint32_t Tag = address >> 12; // Create a bitmask to remove the index and offset
  const uint32_t index = (address >> 4) & ((1 << 8) - 1); // Create a bitmask to remove the tag
  const uint32_t MemAddress = address & ~((1 << 6) - 1); // Create a bitmask to remove the offset
  const uint32_t offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset


  CacheLine *Line = &SimpleCache.lines[index]; 

  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      accessDRAM(MemAddress, &(L1Cache[index << 6]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L1Cache[index << 6]), TempBlock, BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line

    //TODO: fix assignment of data
    if (0 == (address % 8)) { // even word on block
      memcpy(data, &(L1Cache[index << 6 + offset] ), WORD_SIZE); // Perguntar ao professor qual o endereçamento 4 ou 1 bytes?
    } else { // odd word on block
      memcpy(data, &(L1Cache[WORD_SIZE]), WORD_SIZE);
    }
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line

    //TODO: fix assignment of data
    if (!(address % 8)) {   // even word on block
      memcpy(&(L1Cache[0]), data, WORD_SIZE);
    } else { // odd word on block
      memcpy(&(L1Cache[WORD_SIZE]), data, WORD_SIZE);
    }
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
