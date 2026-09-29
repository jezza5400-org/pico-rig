#include "audio/audio.h"
#include <cstdint>
#include <tusb.h>

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