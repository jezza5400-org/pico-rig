#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void write_usb(const uint8_t* data, size_t len);

void usb_update();

#ifdef __cplusplus
}
#endif
