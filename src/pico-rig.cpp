#include "usb_audio.h"
#include <tusb.h>

int main()
{
	tusb_init();

	uint8_t rx_audio[96];

	while (true)
	{
		tud_task();

		// PC -> Pico / radio TX audio
		if (usb_audio_out_streaming())
		{
			uint16_t count =
				usb_audio_read(
					rx_audio,
					sizeof(rx_audio));

			if (count > 0)
			{
				// Send rx_audio to your radio audio output.
			}
		}

		// Pico / radio RX audio -> PC
		if (usb_audio_in_streaming())
		{
			uint8_t tx_audio[96];

			// Fill tx_audio from the radio.
			//
			// radio_audio_read(tx_audio, sizeof(tx_audio));

			usb_audio_write(
				tx_audio,
				sizeof(tx_audio));
		}
	}
}