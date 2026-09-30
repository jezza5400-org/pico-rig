#include "audio.h"

#include <cstdint>

#include "class/cdc/cdc_device.h"
#include <tusb.h>

#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "usb_audio.h"

uint8_t rx_audio[96];

uint8_t previous_line_state = 0;

bool line_state_initialized = false;

bool dtr_high = false;

void init_audio() {
	adc_init();

	// PWM audio output GPIO26

	gpio_init(26);
	gpio_set_dir(26, true);
	gpio_set_function(26, GPIO_FUNC_PWM);

	uint slice_num = pwm_gpio_to_slice_num(26);

	// 10-bit PWM
	// 125MHz / (1023 + 1) = ~122kHz PWM carrier

	pwm_set_wrap(slice_num, 1023);
	pwm_set_enabled(slice_num, true);

	// 50% duty cycle -> audio silence
	pwm_set_gpio_level(26, 512);

	// PTT GPIO27

	gpio_init(27);
	gpio_set_dir(27, true);
	gpio_set_function(27, GPIO_FUNC_SIO);

	gpio_put(27, false);

	// ADC input GPIO28

	adc_gpio_init(28);

	adc_select_input(2);
}

void process_audio() {
	uint8_t line_state = tud_cdc_get_line_state();

	if (!line_state_initialized || line_state != previous_line_state) {
		previous_line_state = line_state;

		line_state_initialized = true;

		dtr_high = (line_state & 0x01) != 0;
	}

	//
	// PC -> Pico -> Radio TX
	//

	if (usb_audio_out_streaming()) {

		uint16_t count = usb_audio_read(rx_audio, sizeof(rx_audio));

		if (count >= 2) {

			gpio_put(27, dtr_high);

			for (uint16_t i = 0; i < count; i += 2) {

				// USB audio is S16_LE

				int16_t sample = static_cast<int16_t>(static_cast<uint16_t>(rx_audio[i]) | (static_cast<uint16_t>(rx_audio[i + 1]) << 8));

				// Convert:
				// -32768 -> 0 PWM
				//      0 -> 512 PWM
				// +32767 -> 1023 PWM
				uint16_t pwm_value = static_cast<uint16_t>((static_cast<int32_t>(sample) + 32768) >> 6);

				pwm_set_gpio_level(26, pwm_value);
			}
		}
	}

	//
	// Radio RX -> Pico -> PC
	//

	if (usb_audio_in_streaming()) {

		int16_t tx_audio[48];

		for (int i = 0; i < 48; i++) {

			uint16_t adc_value = adc_read();

			// ADC:
			//
			// 0      -> negative audio
			// 2048   -> silence
			// 4095   -> positive audio

			int16_t sample = static_cast<int16_t>((static_cast<int32_t>(adc_value) - 2048) << 4);

			tx_audio[i] = sample;
		}

		usb_audio_write(reinterpret_cast<uint8_t*>(tx_audio), sizeof(tx_audio));
	}
}
