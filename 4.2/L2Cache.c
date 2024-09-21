#include "L2Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;

cache l1_cache;
cache l2_cache;

uint8_t* get_cache_line_address(cache* cache, int index) {
    return &(cache->lines[index].data[0]);  // Assuming 64-byte cache lines
}

uint8_t* get_word_address(cache* cache, int index, int offset) {
    return &(cache->lines[index].data[offset * WORD_SIZE]);
}

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

void initCache() { 
    l1_cache.init = 0;
    l2_cache.init = 0; 
}

void accessL2(uint32_t address,  uint8_t *data,  uint32_t mode){
    // Memory will be split into Tag (18 bits) | Index (8 bits) | Offset (6 bits)
    uint32_t tag, index, offset, mem_address;

    // Temporary buffer for blocks
    uint8_t buffer[BLOCK_SIZE];

    // Initialize L2 Cache
    if (l2_cache.init == 0)
    {   
        // Initialize each cache line
        for (size_t i = 0; i < L2_LINES; i++)
        {
            l2_cache.lines[i].valid = 0;
            l2_cache.lines[i].dirty = 0;
            l2_cache.lines[i].tag = 0;
        }
    }

    tag = address >> 14; // Discard the rightmost 14 bits, resulting in tag being the first 18 bits
    index = (address >> 6) & ((1 << 8) - 1); // Create a bitmask to remove the index bits
    offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset
    mem_address = address >> 6; // TODO: não entendo completamente isto, posso confiar que tá bem?

    // Get cache_line pointer
    cache_line* line = &(l2_cache.lines[index]);

    // Access Cache

    // Cache Miss
    if (!line->valid || line->tag != tag) {     
        // Read block from DRAM
        accessDRAM(mem_address, buffer, MODE_READ); 
        
        // If dirty, write back.
        if ((line->valid) && (line->dirty))
        {
            // Write back old block to DRAM
            accessDRAM(mem_address, get_cache_line_address(&l2_cache, index), MODE_WRITE);
        }

        // Copy new block to Cache
        memcpy(get_cache_line_address(&l2_cache, index), buffer, BLOCK_SIZE);

        // Update fields
        line->valid = 1;
        line->tag = tag;
        line->dirty = 0;
    }

    if (mode == MODE_READ) {
        memcpy(data,
               get_word_address(&l2_cache, index, offset),
               WORD_SIZE);

        time += L2_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(get_word_address(&l2_cache, index, offset),
               data, WORD_SIZE);

        time += L2_WRITE_TIME;
        line->dirty = 1;
    }

}

void accessL1(const uint32_t address, uint8_t* data, access_mode mode) {
    // Memory will be split into Tag (18 bits) | Index (8 bits) | Offset (6 bits)
    uint32_t tag, index, offset, mem_address;

    // Temporary buffer for blocks
    uint8_t buffer[BLOCK_SIZE];

    // Initialize L1 Cache
    if (l1_cache.init == 0)
    {   
        // Initialize each cache line
        for (size_t i = 0; i < L2_LINES; i++)
        {
            l1_cache.lines[i].valid = 0;
            l1_cache.lines[i].dirty = 0;
            l1_cache.lines[i].tag = 0;
            for (int k = 0; i < BLOCK_SIZE; ++i) {
                l1_cache.lines[i].data[k] = 0;
            }
        }
    }

    tag = address >> 14; // Discard the rightmost 14 bits, resulting in tag being the first 18 bits
    index = (address >> 6) & ((1 << 8) - 1); // Create a bitmask to remove the index bits
    offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset
    mem_address = address >> 6;

    // Get cache_line pointer
    cache_line* line = &(l1_cache.lines[index]);

    // Access Cache

    // Cache Miss
    if (!line->valid || line->tag != tag) {     
        // Read block from L2
        accessL2(mem_address, buffer, MODE_READ);
        
        // If dirty, write back.
        if ((line->valid) && (line->dirty))
        {
            // Write back old block to L2
            accessL2(mem_address, get_cache_line_address(&l1_cache, index), MODE_WRITE);
        }

        // Copy new block to Cache
        memcpy(get_cache_line_address(&l1_cache, index), buffer, BLOCK_SIZE);

        // Update fields
        line->valid = 1;
        line->tag = tag;
        line->dirty = 0;
    }

     if (mode == MODE_READ) {
        memcpy(data,
               get_word_address(&l1_cache, index, offset),
               WORD_SIZE);

        time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(get_word_address(&l1_cache, index, offset),
               data, WORD_SIZE);

        time += L1_WRITE_TIME;
        line->dirty = 1;
    }


}

void read(uint32_t address, uint8_t* data) {
    accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t* data) {
    accessL1(address, data, MODE_WRITE);
}