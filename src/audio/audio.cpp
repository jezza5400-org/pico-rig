#include "audio.h"
#include <cmath>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

uint8_t rx_audio[96];

void init_audio()
{
    // Initialize GPIO pin 26 for PWM output.
    gpio_init(26);
    gpio_set_dir(26, true);
    gpio_set_function(26, GPIO_FUNC_PWM);

    // Set up PWM frequency and duty cycle.
    uint slice_num = pwm_gpio_to_slice_num(26);
    pwm_set_wrap(slice_num, 4095); // 12-bit resolution
    pwm_set_enabled(slice_num, true);
}

void process_audio() 
{
		// PC -> Pico / radio TX audio
		if (usb_audio_out_streaming())
		{
			uint16_t count =
				usb_audio_read(
					rx_audio,
					sizeof(rx_audio));

			if (count > 0)
			{
				// Using Analog pin 26 (GP29) for audio output to the radio.
				for (uint16_t i = 0; i < count; ++i)
				{
                    // Convert 8-bit unsigned audio to 12-bit unsigned audio for PWM output.
                    uint16_t pwm_value =
                        static_cast<uint16_t>(
                            static_cast<float>(rx_audio[i]) / 255.0f * 4095.0f);

                    // Write the PWM value to the pin.
                    pwm_set_gpio_level(26, pwm_value);
				}
			}
		}

		// Pico / radio RX audio -> PC
		if (usb_audio_in_streaming())
		{
			uint8_t tx_audio[96];

			// Generate sin wave audio for testing.
			for (uint16_t i = 0; i < sizeof(tx_audio); ++i)
			{
				tx_audio[i] =
					static_cast<uint8_t>(
						127.0f * sinf(
							2.0f * 3.14159f * 440.0f * i / 48000.0f) + 128.0f); // 440 Hz sine wave at 48 kHz sample rate.
			}

			usb_audio_write(
				tx_audio,
				sizeof(tx_audio));
		}

}
