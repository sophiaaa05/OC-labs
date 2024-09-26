#include "2wL2Cache.h"

uint8_t DRAM[DRAM_SIZE] = {0};
uint8_t l1_cache_data[L1_SIZE];
uint8_t l2_cache_data[L2_SIZE];
uint32_t time;

cache l1_cache;
cache l2_cache;

cache_line* get_cache_line(cache* cache, int index) {
    return &(cache->lines[index]);
}

uint8_t* get_block(uint8_t* cache, int index) {
    return &(cache[index<<6]);
}

uint8_t* get_word(uint8_t* cache, int index, int offset) {
    return &(cache[(index * BLOCK_SIZE) + offset]);
}

cache_line* select_cache_line(uint32_t set_index, uint32_t tag) {
    uint32_t set_start = set_index * L2_WAYS;
    cache_line* lru_line = &(l2_cache.lines[set_start]);
    uint32_t lru_counter = UINT32_MAX;  // Start with maximum possible value

    // Iterate through all ways in the set
    for (int i = 0; i < L2_WAYS; i++) {
        cache_line* current_line = &(l2_cache.lines[set_start + i]);

        // Case 1: Line is invalid (free) - use this line
        if (!current_line->valid) {
            return current_line;
        }

        // Case 2: Tag match (cache hit) - use this line
        if (current_line->tag == tag) {
            return current_line;
        }

        // Track LRU: Lower counter value means less recently used
        if (current_line->lru < lru_counter) {
            lru_counter = current_line->lru;
            lru_line = current_line;
        }
    }

    // If we reach here, all lines are valid and there's no tag match
    // Return the least recently used line
    return lru_line;
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
            l2_cache.lines[i].tag = 0;
            l2_cache.lines[i].lru = 0;
        }
        l2_cache.init = 1;
    }

    tag = address >> 14; // Discard the rightmost 14 bits, resulting in tag being the first 18 bits
    index = (address >> 6) & ((1 << 8) - 1); // Create a bitmask to remove the index bits
    offset = address & ((1 << 6) - 1); // Create a bitmask to remove the offset
    mem_address = address >> 6;

    // Get cache_line
    cache_line* line = select_cache_line(index, tag);

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

    for (int i = 0; i < L2_LINES; i++) {
        l2_cache.lines[i].lru++;
    }

    line->lru = 0;

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
        accessL2(address, buffer, MODE_READ);

        // Check if we are going to a new block, if so reads from RAM
        if ((address & BLOCK_SIZE - 1) == 0 && (mem_address) % BLOCK_SIZE != 0 && address != 0) 
        { 
            accessDRAM(address, buffer, MODE_READ);
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