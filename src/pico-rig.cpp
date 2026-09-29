#include "audio/audio.h"
#include <tusb.h>
#include "pico/stdlib.h"
#include "usb/civ.h"

int main()
{
	tusb_init();
	init_audio();

	uint8_t civ_buffer[32];
	size_t civ_buffer_len = 0;

	while (true)
	{
		tud_task();
		uint8_t buffer[64];
		size_t bytes_read = tud_cdc_read(buffer, sizeof(buffer));

		for (size_t i = 0; i < bytes_read; ++i)
		{
			uint8_t byte = buffer[i];

			if (civ_buffer_len == 0 && byte != CIV_START)
				continue;

			if (byte == CIV_START && civ_buffer_len >= 2 &&
			    civ_buffer[civ_buffer_len - 1] != CIV_START)
				civ_buffer_len = 0;

			if (civ_buffer_len >= sizeof(civ_buffer))
				civ_buffer_len = 0;

			civ_buffer[civ_buffer_len++] = byte;

			if (civ_buffer_len >= 6 && byte == CIV_STOP)
			{
				std::array<uint8_t, 32> frame{};
				std::copy(civ_buffer,
					  civ_buffer + civ_buffer_len,
					  frame.begin());
				process_civ_command(frame, civ_buffer_len);
				civ_buffer_len = 0;
			}
		}

		process_audio();
	}
}