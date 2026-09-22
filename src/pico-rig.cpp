#include "usb_audio.h"
#include <tusb.h>
#include <cmath>
#include "pico/stdlib.h"

uint8_t rx_audio[96];

void usb_serial_write(const char* str)
{
	if (!tud_cdc_connected())
		return;

	while (*str)
	{
		tud_cdc_write_char(*str++);
	}
	tud_cdc_write_flush();
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
					if(rx_audio[i] > 128)
						usb_serial_write("[AUDIO]: HIGH\n");
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

int main()
{
	tusb_init();

	while (true)
	{
		tud_task();

		process_audio();
	}
}