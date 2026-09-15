#include "hardware/psram.h"
#include "pico/stdlib.h"
#include <stdio.h>

int not_main() {
	stdio_init_all();

	while (!stdio_usb_connected()) {
		sleep_ms(10);
	}

	sleep_ms(500);

	printf("\n============================================\n");
	printf("  Pimoroni Pico Plus 2W PSRAM Status Tool\n");
	printf("============================================\n");

	size_t total_psram_bytes = psram_get_size();
	printf("Detected PSRAM Array: %zu bytes (%.1f MB)\n", total_psram_bytes, (float)total_psram_bytes / (1024.0f * 1024.0f));

	uintptr_t psram_base_addr = 0x11000000;

	printf("Validating base address structure... ");
	if (psram_check_address((void *)psram_base_addr)) {
		printf("SUCCESS (Accessible)\n");

		volatile uint32_t *test_ptr = (volatile uint32_t *)psram_base_addr;

		printf("Testing hardware writes...\n");
		test_ptr[0] = 0xDEADBEEF;
		test_ptr[1] = 0xCAFEBABE;
		test_ptr[256] = 0x12345678; // Offset test

		printf("Testing hardware reads...\n");
		printf("  Address [0x11000000]: 0x%08X\n", test_ptr[0]);
		printf("  Address [0x11000004]: 0x%08X\n", test_ptr[1]);
		printf("  Address [0x11000400]: 0x%08X\n", test_ptr[256]);
	} else {
		printf("FAILED (Address space unmapped by SDK)\n");
	}

	printf("\nDone. Entering infinite loop.\n");
	while (true) {
		tight_loop_contents();
	}
}
