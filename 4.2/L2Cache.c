#include "L2Cache.h"

uint8_t DRAM[DRAM_SIZE] = {0};
uint8_t l1_cache_data[L1_SIZE];
uint8_t l2_cache_data[L2_SIZE];
uint32_t time;

cache l1_cache;
cache l2_cache;

int counter = 0;

cache_line* get_cache_line(cache* cache, int index) {
    return &(cache->lines[index]);
}

uint8_t* get_block(uint8_t* cache, int index) {
    return &(cache[index<<6]);
}

uint8_t* get_word(uint8_t* cache, int index, int offset) {
    return &(cache[(index * BLOCK_SIZE) + offset]);
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
    uint8_t buffer[BLOCK_SIZE] = {0};

    // Initialize L2 Cache
    if (l2_cache.init == 0)
    {   
        // Initialize each cache line
        for (size_t i = 0; i < L2_LINES; i++) 
        {
            l2_cache.lines[i].valid = false;
            l2_cache.lines[i].dirty = false;
            l2_cache.lines[i].tag = 0;
        }
        l2_cache.init = 1;
    }

    tag = address >> 14; // Discard the rightmost 14 bits, resulting in tag being the first 18 bits
    index = (address >> 6) & ((1 << 8) - 1); // Create a bitmask to remove the index bits
    offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset
    mem_address = address >> 6;

    // Get cache_line pointer
    cache_line* line = get_cache_line(&l2_cache, index);

    // Cache Miss
    if (!(line->valid) || line->tag != tag) {
        // Read block from DRAM
        accessDRAM(mem_address, buffer, MODE_READ);
       
        // If dirty, write back.
        if ((line->valid) && (line->dirty))
        {
            // Write back old block to DRAM
            accessDRAM(mem_address, get_block(l2_cache_data, index), MODE_WRITE);
        }

        // Copy new block to Cache
        memcpy(get_block(l2_cache_data, index), buffer, BLOCK_SIZE);

        // Update fields
        line->valid = true;
        line->tag = tag;
        line->dirty = false;
    }

    if (mode == MODE_READ) {
        memcpy(data,
               get_word(l2_cache_data, index, offset),
               WORD_SIZE);

        time += L2_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(get_word(l2_cache_data, index, offset),
               data, WORD_SIZE);

        time += L2_WRITE_TIME;
        line->dirty = true;
    }

}

void accessL1(const uint32_t address, uint8_t* data, access_mode mode) {
    // Memory will be split into Tag (18 bits) | Index (8 bits) | Offset (6 bits)
    uint32_t tag, index, offset, mem_address;

    // Temporary buffer for blocks
    uint8_t buffer[BLOCK_SIZE] = {0};

    // Initialize L1 Cache
    if (l1_cache.init == 0)
    {   
        // Initialize each cache line
        for (size_t i = 0; i < L1_LINES; i++)
        {
            l1_cache.lines[i].valid = false;
            l1_cache.lines[i].dirty = false;
            l1_cache.lines[i].tag = 0;
        }
        l1_cache.init = 1;
    }

    tag = address >> 14; // Discard the rightmost 14 bits, resulting in tag being the first 18 bits
    index = (address >> 6) & ((1 << 8) - 1); // Create a bitmask to remove the index bits
    offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset
    mem_address = address >> 6;

    // Get cache_line pointer
    cache_line* line = get_cache_line(&l1_cache, index);

    // Cache Miss
    if (!(line->valid) || line->tag != tag) {
        // Read block from L2 and store in buffer
        
        // Access L2 if address is not a multiple of 64
        accessL2(mem_address, buffer, MODE_READ); 

        // Check if we are going to a knew block, is so reads from RAM
        if ((address & 63) == 0 && (address >> 6) % 64 != 0) 
        { 
            accessDRAM(mem_address, buffer, MODE_READ);
        } 
        

        // If dirty, write back.
        if ((line->valid) && (line->dirty))
        {
            // Write back old block to L2
            accessL2(mem_address, get_block(l1_cache_data, index), MODE_WRITE);
        }

        // Copy read block in buffer to block in Cache
        memcpy(get_block(l1_cache_data, index), buffer, BLOCK_SIZE);

        // Update fields
        line->valid = true;
        line->tag = tag;
        line->dirty = false;
    }

     if (mode == MODE_READ) {
        memcpy(data,
               get_word(l1_cache_data, index, offset),
               WORD_SIZE);

        time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(get_word(l1_cache_data, index, offset),
               data, WORD_SIZE);

        time += L1_WRITE_TIME;
        line->dirty = true;
    }


}

void read(uint32_t address, uint8_t* data) {
    accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t* data) {
    accessL1(address, data, MODE_WRITE);
}