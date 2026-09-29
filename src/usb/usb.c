#include "usb.h" 
#include "tusb.h"

void write_usb(const uint8_t *data, size_t len)
{
    if (!tud_cdc_connected())
        return;

    size_t written = 0;
    while (written < len)
    {
        uint32_t available = tud_cdc_write_available();
        if (available == 0)
        {
            tud_task();
            continue;
        }

        size_t chunk = len - written;
        if (chunk > available)
            chunk = available;

        written += tud_cdc_write(data + written, chunk);
        tud_cdc_write_flush();
    }
}

void read_usb(uint8_t *buffer, size_t len)
{
    if (tud_cdc_connected())
    {
        tud_cdc_read(buffer, len);
    }
}