#pragma once

#include <cstdint>

bool usb_audio_in_streaming();
bool usb_audio_out_streaming();

bool usb_audio_in_muted();
bool usb_audio_out_muted();

int16_t usb_audio_in_volume();
int16_t usb_audio_out_volume();

uint32_t usb_audio_in_sample_rate();
uint32_t usb_audio_out_sample_rate();

uint16_t usb_audio_read(
    void* buffer,
    uint16_t buffer_size);

uint16_t usb_audio_write(
    const void* buffer,
    uint16_t buffer_size);

uint16_t usb_audio_bytes_per_frame();