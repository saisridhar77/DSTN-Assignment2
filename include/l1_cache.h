#ifndef L1_CACHE_H

#include <stdint.h>
#include <stdbool.h>

/* Cache Parameters */
#define L1_NUM_SETS 64
#define L1_NUM_WAYS 4
#define L1_LINE_SIZE 16

//--- Bit manipulation macros for breaking up address ---------------------------------------------------------------------------------

// Offset is the same for virtual addr and physical addr (bits 0-3)
#define L1_OFFSET_MASK 0x0000000F 
#define GET_L1_OFFSET(addr) ((addr) & L1_OFFSET_MASK)

// Index comes from physical addr (bits 4-9)
#define L1_INDEX_MASK  0x000003F0 
#define GET_L1_PHY_INDEX(pa) (((pa) & L1_INDEX_MASK) >> 4)

// Tag comes from virtual addr (bits 10-31)
#define L1_TAG_MASK    0xFFFFFC00 
#define GET_L1_VIRT_TAG(va)    (((va) & L1_TAG_MASK) >> 10)

//--- Structures that make up the cache -----------------------------------------------------------------------------------------------

// Structure for an L1 cache line
typedef struct {
    bool valid;
    uint32_t tag;       // Virtual Tag
    uint8_t data[L1_LINE_SIZE];
} L1_Line;

// Structure for a single set made of 4 ways
typedef struct {
    L1_Line ways[L1_NUM_WAYS];
    uint16_t lru_matrix; //4x4 matrix so 16 bits
    uint8_t predicted_way; //Stores the last way that was accessed
} L1_Set;

// Structure for L1 cache
typedef struct {
    L1_Set sets[L1_NUM_SETS];
    
    // Statistics
    unsigned int hits;
    unsigned int misses;
    unsigned int way_prediction_success;
    unsigned int way_prediction_failure;
} L1_Cache;

//--- Public functions ----------------------------------------------------------------------------------------------------------------

// Initialize a new L1 cache
L1_Cache* l1_cache_create(void);

// Free the memory allocated for the L1 cache
void l1_cache_destroy(L1_Cache* cache);

/* return value: Returns false only if there is a hard Page Fault (page not in physical memory)
 * cache: Pointer to L1 cache that we want to access
 * virtual_addr: Virtual addr of the memory we want to access (conversions to physical addr are done internally)
 * is_write: 1 -> memory store, 0 -> memory load
 * out_data: Pointer to where the data should be stored for a memory load (ignored if it's a store)
 */
bool l1_cache_access(L1_Cache* cache, uint32_t virtual_addr, bool is_write, uint8_t* out_data);

#endif