#pragma once

#define SCALE_FIRMWARE_VERSION "1.2.9"
static_assert(sizeof(SCALE_FIRMWARE_VERSION) <= 9, "Firmware version must fit in the radio packet");
