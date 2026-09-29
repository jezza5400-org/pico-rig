#include "audio/audio.h"
#include <tusb.h>
#include "pico/stdlib.h"

int main()
{
	tusb_init();
	init_audio();

	while (true)
	{
		tud_task();

		process_audio();
	}
}