# Memory Subsystem Implementation (C, ADT-based)

Design notes for implementing a TLB + L1 + L2 + Main Memory subsystem in C
using the opaque-struct ADT pattern.

## Given Configuration

- **TLB**: PID-tagged, 32 entries. Invalidation on process termination.
- **L1 Cache**: 4KB, 16B line, 4-way set associative, way-predicted.
  Virtually tagged / physically indexed (VT-PI). Write-through, look-through
  (no write-allocate). Replacement: LRU square matrix.
- **L2 Cache**: 32KB, 32B line, 8-way set associative. Write-back, look-aside
  (write-allocate on miss). Replacement: LRU counter.
- **Main Memory**: 32MB, pure paging, LRU page replacement.

## Architecture: One ADT per Component

Each component is a separate module exposing only an opaque handle
(`typedef struct X* X_t`) plus a small set of verbs. Internal layout
(struct fields, replacement-policy bookkeeping) is hidden inside the `.c`
file — callers never touch it directly.

```
tlb.h / tlb.c
l1_cache.h / l1_cache.c
l2_cache.h / l2_cache.c
main_memory.h / main_memory.c
secondary_mem.h / secondary_mem.c   // minimal stub, see below
mem_subsystem.h / mem_subsystem.c   // orchestrator
```

### TLB
Fully-associative, 32 entries. Fields: `vpn`, `pfn`, `pid`, `valid`.
API: `tlb_create`, `tlb_lookup(pid, vpn)`, `tlb_insert`, `tlb_invalidate_pid`
(called on process termination).

### L1 Cache (the tricky one)
Because it's VT-PI, the **physical address's index bits** are needed to
probe the set, but the **virtual tag** is used for the compare — so a TLB
lookup happens first just to get the PFN, even though the tag itself stays
virtual. 4 ways per set, way-predicted (a stored predicted-way is checked
before comparing all 4 tags). Write-through + no-write-allocate.
API: `l1_lookup(vtag, pindex)`, `l1_write`, `l1_fill`.

### L2 Cache
Standard PIPT (physically indexed, physically tagged), 8-way.
Write-back + write-allocate ("look-aside"). LRU counter replacement.
API: `l2_lookup(ptag, pindex)`, `l2_write`.

### Main Memory / Paging
Owns the per-PID page table and 32MB of frames. On a TLB miss, walks the
page table; on a page fault, evicts an LRU frame and loads the needed page.
API: `mm_create`, `mm_translate(pid, vpn)`.

### Orchestrator
`mem_access(pid, vaddr, is_write, data)` implements the full access path:
TLB (or page-table walk + TLB insert on miss) → split address into
vtag/pindex → L1 lookup → on miss, L2 lookup → on miss, main memory read +
L2 fill + L1 fill.

## Does Main-Memory LRU Require a Secondary Memory?

Conceptually yes — LRU eviction only makes sense if evicted pages go
somewhere and faulted-in pages come from somewhere. Practically, unless the
assignment gives explicit disk-latency numbers to use in timing
calculations, a **minimal stub ADT** is enough — no need to model real disk
timing or layout:

```c
void secondary_store_page(uint32_t vpn, const uint8_t *data);
void secondary_load_page(uint32_t vpn, uint8_t *buf);
```

Backed by a flat array, a file, or dummy/zero-filled data. If the spec
*does* give disk latency as one of the given numbers, that's the signal to
model it as a real timed component instead.

## LRU Implementation Options

Three approaches, matched to where each is used in this design:

| Component     | Structure       | Why |
|---------------|-----------------|-----|
| TLB           | Doubly-linked list (MRU head / LRU tail) + lookup table | O(1) touch/victim, good for fully-associative 32-entry sets |
| L1 Cache      | Square/bit matrix (N×N per set) | Matches spec exactly; classic hardware LRU for small N (4-way) |
| L2 Cache      | Counter per way, incremented on every access except the hit | Matches spec exactly; simple O(ways) per access, fine for 8-way |
| Main Memory   | Doubly-linked list over frames | Scales better than matrix/counter (O(n²)/O(n)) for a large frame pool |

**Counter-based** (L2): increment all counters in the set on access, reset
the touched way to 0; victim = highest counter.

**Square-matrix** (L1): N×N bit matrix `M[i][j]` = "way i more recently
used than way j". On touch of way k: set row k to 1, clear column k to 0.
Victim = the way whose row is all zeros.

**Stack/list-based** (TLB, main memory): standard MRU-head/LRU-tail
doubly-linked list with a direct-lookup array for O(1) touch and O(1)
victim (list tail).

Keep each LRU implementation as a private helper module
(`lru_counter.c`, `lru_matrix.c`, `lru_list.c`) used internally by the
relevant ADT — never exposed to the orchestrator.

## Open Questions / Next Steps

- Confirm whether the assignment provides hit/miss-penalty numbers
  (L1/L2/memory/disk access times) — if disk latency is given, secondary
  memory needs to be modeled as a real timed component, not a stub.
- Flesh out `l1_lookup`/`l1_fill` integration with the LRU square-matrix
  and way-prediction logic (the most involved part of the design).
- Decide on TLB's own replacement policy structure (list-based recommended
  above, spec doesn't name one explicitly).
