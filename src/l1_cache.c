#include "l1_cache.h"
#include <stdlib.h>

//--- Helper functions ----------------------------------------------------------------------------------------------------------------

/* Function that updates the LRU square matrix after an access
 * When way 'k' gets accessed, row k is made 1 and column k is made 0
 * Row k: bits (k*4) to (k*4 + 3)
 * Column k: bits k, k+4, k+8, k+12
*/
static void update_lru_matrix (L1_Set* set, uint8_t accessed_way){
    // Setting bits in row 'k'
    uint16_t row_mask = 0xF << (accessed_way * 4);
    set->lru_matrix |= row_mask;

    // Clearing bits in column k
    uint16_t col_mask = 0x1111 << accessed_way;
    set->lru_matrix &= ~col_mask;

    return;
}

/* Function that finds which way to evict (which is the way with row as with all 0s)
 * Returns the way number to evict
 */
static uint8_t get_eviction_way (L1_Set* set){
    for (uint8_t i = 0; i < L1_NUM_WAYS; i++){
        uint8_t row_val = ((set->lru_matrix) >> (i*4)) & 0xF;
        if (row_val == 0){
            return i;
        }
    }

    return 0; // should never reach here
}

//--- Public functions ----------------------------------------------------------------------------------------------------------------

L1_Cache* l1_cache_create(void){
    L1_Cache* cache = (L1_Cache*)calloc(1, sizeof(L1_Cache));
    return cache;
}

void l1_cache_destroy(L1_Cache* cache){
    if (cache != NULL){
        free(cache);
    }
    return;
}