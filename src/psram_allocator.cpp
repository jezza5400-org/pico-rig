#include "psram_allocator.h"
#include <stdint.h>

static uintptr_t psram_heap_current = 0x11000000;
const uintptr_t PSRAM_HEAP_END = 0x11000000 + (8 * 1024 * 1024);

void *psram_malloc(size_t size) {
	// Align allocations to 4-byte boundaries for the 32-bit bus
	size = (size + 3) & ~3;

	if (psram_heap_current + size > PSRAM_HEAP_END) {
		return nullptr; // Out of memory
	}

	void *allocated_ptr = (void *)psram_heap_current;
	psram_heap_current += size;
	return allocated_ptr;
}
