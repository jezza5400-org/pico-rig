#pragma once
#include <cstdint>
#include <cstddef>
#include <array>

#define CIV_START 0xFE
#define CIV_STOP 0xFD
#define RIG_ADDR 0xA4 // IC-705 default CI-V address
#define CIV_ACK 0xFB  // Command OK
#define CIV_NAK 0xFA  // Command Error

static constexpr char HEX_LOOKUP[] = "0123456789ABCDEF";

void process_civ_command(const std::array<uint8_t, 32> &frame, size_t frame_len);