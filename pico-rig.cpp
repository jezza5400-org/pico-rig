#include "hardware/uart.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <array>
#include <cstring>
#include <stdio.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// Icom CI-V Protocol Bytes
#define CIV_START 0xFE
#define CIV_STOP 0xFD
#define RIG_ADDR 0xA4 // IC-705 default CI-V address
#define CIV_ACK 0xFB  // Command OK
#define CIV_NAK 0xFA  // Command Error

static constexpr char HEX_LOOKUP[] = "0123456789ABCDEF";

// Dummy State
uint64_t current_freq_hz = 14200000;
uint8_t current_mode = 0x01;
uint8_t current_filter = 0x01;
bool ptt_state = false;

void set_ptt(bool state) {
	ptt_state = state;
	gpio_put(PICO_DEFAULT_LED_PIN, ptt_state);
}

void send_civ_frame(uint8_t target_addr, uint8_t cmd, const uint8_t *data, size_t data_len) {
	uint8_t frame[32];
	size_t idx = 0;

	frame[idx++] = CIV_START;
	frame[idx++] = CIV_START;
	frame[idx++] = target_addr; // Echo back the host address
	frame[idx++] = RIG_ADDR;	// Rig address (0xA4)
	frame[idx++] = cmd;
	for (size_t i = 0; i < data_len; i++) {
		frame[idx++] = data[i];
	}
	frame[idx++] = CIV_STOP;

	for (size_t i = 0; i < idx; i++) {
		putchar_raw(frame[i]);
	}
	stdio_flush();
}

void send_ack(uint8_t target_addr) {
	uint8_t frame[6] = {CIV_START, CIV_START, target_addr, RIG_ADDR, CIV_ACK, CIV_STOP};
	for (size_t i = 0; i < 6; i++) {
		putchar_raw(frame[i]);
	}
	stdio_flush();
}

void send_nak(uint8_t target_addr) {
	uint8_t frame[6] = {CIV_START, CIV_START, target_addr, RIG_ADDR, CIV_NAK, CIV_STOP};
	for (size_t i = 0; i < 6; i++) {
		putchar_raw(frame[i]);
	}
	stdio_flush();
}

uint64_t bcd_to_freq(const uint8_t *bcd) {
	uint64_t freq = 0;
	uint64_t mult = 1;
	for (int i = 0; i < 5; i++) {
		freq += (bcd[i] & 0x0F) * mult;
		mult *= 10;
		freq += ((bcd[i] >> 4) & 0x0F) * mult;
		mult *= 10;
	}
	return freq;
}

void freq_to_bcd(uint64_t freq, uint8_t *bcd) {
	for (int i = 0; i < 5; i++) {
		uint8_t low = freq % 10;
		freq /= 10;
		uint8_t high = freq % 10;
		freq /= 10;
		bcd[i] = (high << 4) | low;
	}
}

void process_civ_command(const std::array<uint8_t, 32> &frame, size_t frame_len) {
	if (frame_len < 6) return;

	if (frame[0] != CIV_START || frame[1] != CIV_START) return;
	if (frame[2] != RIG_ADDR && frame[2] != 0x00) return;

	uint8_t host_addr = frame[3];
	uint8_t cmd = frame[4];

	switch (cmd) {
		// Read Frequency (0x03)
		case 0x03: {
			uint8_t bcd[5];
			freq_to_bcd(current_freq_hz, bcd);
			send_civ_frame(host_addr, 0x03, bcd, 5);
			break;
		}

		// Set Frequency (0x05)
		case 0x05: {
			if (frame_len >= 11 && frame[10] == CIV_STOP) {
				current_freq_hz = bcd_to_freq(&frame[5]);
				send_ack(host_addr);
			} else {
				send_nak(host_addr);
			}
			break;
		}

		// Read Operating Mode (0x04)
		case 0x04: {
			uint8_t mode_data[2] = {current_mode, current_filter};
			send_civ_frame(host_addr, 0x04, mode_data, 2);
			break;
		}

		// Set Operating Mode (0x06)
		case 0x06: {
			if (frame_len >= 7) {
				current_mode = frame[5];
				if (frame_len >= 8 && frame[6] != CIV_STOP) {
					current_filter = frame[6];
				}
				send_ack(host_addr);
			} else {
				send_nak(host_addr);
			}
			break;
		}

		// Read Rig ID (0x19 0x00)
		case 0x19: {
			uint8_t id_resp[2] = {0x00, RIG_ADDR}; // Model ID 0xA4 for IC-705
			send_civ_frame(host_addr, 0x19, id_resp, 2);
			break;
		}

		// Transmit / PTT Control (0x1C)
		case 0x1C: {
			if (frame_len >= 7 && frame[5] == 0x00) {
				if (frame_len == 7) {
					uint8_t state_byte = ptt_state ? 0x01 : 0x00;
					uint8_t resp[2] = {0x00, state_byte};
					send_civ_frame(host_addr, 0x1C, resp, 2);
				} else if (frame_len >= 8) {
					set_ptt(frame[6] == 0x01);
					send_ack(host_addr);
				}
			} else {
				send_nak(host_addr);
			}
			break;
		}

		// Transmit / VFO status query (0x25)
		case 0x25: {
			if (frame_len >= 7) {
				uint8_t subcmd = frame[5];
				if (frame_len == 7) {
					uint8_t resp[2] = {subcmd, 0x00};
					send_civ_frame(host_addr, 0x25, resp, 2);
				} else {
					send_ack(host_addr);
				}
			} else {
				send_nak(host_addr);
			}
			break;
		}

		// Level Control / RF Power query (0x14)
		case 0x14: {
			if (frame_len >= 7) {
				uint8_t subcmd = frame[5];
				if (frame_len == 7) {
					uint8_t resp[3] = {subcmd, 0x01, 0x28};
					send_civ_frame(host_addr, 0x14, resp, 3);
				} else {
					send_ack(host_addr);
				}
			} else {
				send_nak(host_addr);
			}
			break;
		}

		// Misc / Transceiver Status (0x1A)
		case 0x1A: {
			if (frame_len >= 7) {
				uint8_t subcmd = frame[5];
				if (frame_len == 7) {
					uint8_t resp[2] = {subcmd, 0x00};
					send_civ_frame(host_addr, 0x1A, resp, 2);
				} else {
					send_ack(host_addr);
				}
			} else {
				send_nak(host_addr);
			}
			break;
		}

		default:
			// Send NAK so host knows the command isn't supported
			send_nak(host_addr);
			break;
	}
}

int main() {
	stdio_init_all();

	// Initialize UART for debugging
	uart_init(uart0, 115200);
	gpio_set_function(0, GPIO_FUNC_UART);
	gpio_set_function(1, GPIO_FUNC_UART);

	// Initialize Onboard LED GPIO
	gpio_init(PICO_DEFAULT_LED_PIN);
	gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
	gpio_put(PICO_DEFAULT_LED_PIN, 0); // Start LED OFF

	std::array<uint8_t, 32> buffer{0};
	size_t buf_len = 0;

	while (true) {
		int c = stdio_getchar_timeout_us(1000);

		if (c != PICO_ERROR_TIMEOUT) {
			uint8_t byte = static_cast<uint8_t>(c);

			// Re-synchronize frame buffer on receiving start byte
			if (byte == CIV_START) {
				if (buf_len >= 2 && buffer[buf_len - 1] != CIV_START) {
					buf_len = 0;
					uart_puts(uart0, "\r\n");
				}
			}

			// Prevent buffer overflow
			if (buf_len >= buffer.size()) {
				buf_len = 0;
				uart_puts(uart0, "\r\n");
			}

			buffer[buf_len++] = byte;

			// Frame complete
			if (byte == CIV_STOP) {
				process_civ_command(buffer, buf_len);

				std::array<char, 128> print_buf;
				size_t idx = 0;

				for (size_t i = 0; i < buf_len; i++) {
					uint8_t b = buffer[i];
					print_buf[idx++] = HEX_LOOKUP[b >> 4];
					print_buf[idx++] = HEX_LOOKUP[b & 0x0F];
					print_buf[idx++] = ' ';
				}
				print_buf[idx++] = '\r';
				print_buf[idx++] = '\n';
				print_buf[idx++] = '\0';

				uart_puts(uart0, print_buf.data());

				buf_len = 0;
			}
		}
	}

	return 0;
}
