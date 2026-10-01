#include "audio/audio.h"
#include <cstdint>
#include <tusb.h>

#include "pico/cyw43_arch.h"
#include "hardware/uart.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"

int main() {
	tusb_init();
	cyw43_arch_init();
	init_audio();

	stdio_init_all();
	uart_init(uart0, 115200);
	gpio_set_function(0, GPIO_FUNC_UART);
	gpio_set_function(1, GPIO_FUNC_UART);


	while (true) {
		tud_task();
		process_audio();
	}
}
