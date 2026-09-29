#include "audio.h"
#include <cstdint>

#include <tusb.h>
#include "class/cdc/cdc_device.h"

#include "usb_audio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

uint8_t rx_audio[96];
uint8_t previous_line_state = 0;
bool line_state_initialized = false;

bool dts_high = false;

void init_audio()
{
    adc_init();

    // Initialize GPIO pin 26 for PWM output.
    gpio_init(26);
    gpio_set_dir(26, true);
    gpio_set_function(26, GPIO_FUNC_PWM);

    // Set up PWM frequency and duty cycle.
    uint slice_num = pwm_gpio_to_slice_num(26);
    pwm_set_wrap(slice_num, 4095); // 12-bit resolution
    pwm_set_enabled(slice_num, true);

    // Initialize GPIO pin 27 for ptt output
    gpio_init(27);
    gpio_set_dir(27, true);
    gpio_set_function(27, GPIO_FUNC_SIO);
    gpio_put(27, false); // Set PTT low initially

    // Read from GPIO 28 for ADC input (microphone input)
    adc_gpio_init(28);
    adc_select_input(2); // GPIO 28 corresponds to ADC input 2
}

void process_audio() 
{
        uint8_t line_state = tud_cdc_get_line_state();
        if (!line_state_initialized || line_state != previous_line_state)
        {
        	previous_line_state = line_state;
        	line_state_initialized = true;

            dts_high = (line_state & 0x01) != 0; // Update dts_high based on DTR state
        }
	

		// PC -> Pico / radio TX audio
		if (usb_audio_out_streaming())
		{
			uint16_t count =
				usb_audio_read(
					rx_audio,
					sizeof(rx_audio));

			if (count > 0)
			{
				// Only process audio if DTS is high
				if(dts_high)
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

                    if (rx_audio[i] > 128) {
                        gpio_put(27, true); // Set PTT high
                    } else {
                        gpio_put(27, false); // Set PTT low
                    }
				}
                }
			}
		}

		// Pico / radio RX audio -> PC
		if (usb_audio_in_streaming())
		{
			uint8_t tx_audio[96];

			for (uint16_t i = 0; i < sizeof(tx_audio); ++i)
			{
                //TODO: FIX THIS ASS IMPLEMNTAION, THIS WILL GIVE DISTORTION
                const float conversion_factor = 3.3f / (1 << 12);
                uint16_t result = adc_read();

                tx_audio[i] = static_cast<uint8_t>(result * conversion_factor * 255.0f / 3.3f);
            }

			usb_audio_write(
				tx_audio,
				sizeof(tx_audio));
		}

}
