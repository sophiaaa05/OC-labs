#include "L1Cache.h" // Use previous implementation of L1Cache
#include "L2Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;

cache l2_cache;

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

/*********************** L2 cache *************************/

void initCache() { l2_cache.init = 0; }

void accessL2(uint32_t address,  uint8_t *data,  uint32_t mode){

}