#include "tusb.h"
#include "class/audio/audio.h"

#include <algorithm>
#include <cstdint>

// -----------------------------------------------------------------------------
// Your USB interface numbers
// -----------------------------------------------------------------------------

#ifndef ITF_NUM_AUDIO_CONTROL
#define ITF_NUM_AUDIO_CONTROL 2
#endif

#ifndef ITF_NUM_AUDIO_STREAMING_OUT
#define ITF_NUM_AUDIO_STREAMING_OUT 3
#endif

#ifndef ITF_NUM_AUDIO_STREAMING_IN
#define ITF_NUM_AUDIO_STREAMING_IN 4
#endif

// -----------------------------------------------------------------------------
// Your UAC1 entity IDs
// -----------------------------------------------------------------------------

static constexpr uint8_t AUDIO_ENTITY_MIC_INPUT       = 1;
static constexpr uint8_t AUDIO_ENTITY_MIC_FEATURE     = 2;
static constexpr uint8_t AUDIO_ENTITY_MIC_OUTPUT      = 3;

static constexpr uint8_t AUDIO_ENTITY_SPK_INPUT       = 4;
static constexpr uint8_t AUDIO_ENTITY_SPK_FEATURE     = 5;
static constexpr uint8_t AUDIO_ENTITY_SPK_OUTPUT      = 6;

// -----------------------------------------------------------------------------
// Endpoint addresses
// -----------------------------------------------------------------------------

static constexpr uint8_t AUDIO_EP_OUT = 0x03;
static constexpr uint8_t AUDIO_EP_IN  = 0x83;

// -----------------------------------------------------------------------------
// Audio format
// -----------------------------------------------------------------------------

static constexpr uint32_t AUDIO_SAMPLE_RATE = 48000;
static constexpr uint8_t  AUDIO_CHANNELS    = 1;
static constexpr uint8_t  AUDIO_BITS        = 16;
static constexpr uint8_t  AUDIO_BYTES       = AUDIO_BITS / 8;

// 48 kHz × 1 channel × 16-bit = 96 bytes every 1 ms USB frame.
static constexpr uint16_t AUDIO_BYTES_PER_FRAME =
    (AUDIO_SAMPLE_RATE / 1000) *
    AUDIO_CHANNELS *
    AUDIO_BYTES;

// -----------------------------------------------------------------------------
// Audio state
// -----------------------------------------------------------------------------

static bool audio_in_streaming  = false;
static bool audio_out_streaming = false;

static bool audio_in_mute  = false;
static bool audio_out_mute = false;

// UAC1 volume is signed 16-bit, expressed in 1/256 dB.
//
// 0 dB      = 0
// -60 dB    = -15360
// 1 dB      = 256
//
static constexpr int16_t AUDIO_VOLUME_MIN = -15360;
static constexpr int16_t AUDIO_VOLUME_MAX = 0;
static constexpr int16_t AUDIO_VOLUME_RES = 256;

static int16_t audio_in_volume  = 0;
static int16_t audio_out_volume = 0;

static uint32_t audio_in_sample_rate  = AUDIO_SAMPLE_RATE;
static uint32_t audio_out_sample_rate = AUDIO_SAMPLE_RATE;

// Persistent control-transfer buffer.
// Do not use a local array for tud_control_xfer().
static uint8_t audio_control_response[3];

// -----------------------------------------------------------------------------
// Public state accessors
// -----------------------------------------------------------------------------

bool usb_audio_in_streaming()
{
    return audio_in_streaming;
}

bool usb_audio_out_streaming()
{
    return audio_out_streaming;
}

bool usb_audio_in_muted()
{
    return audio_in_mute;
}

bool usb_audio_out_muted()
{
    return audio_out_mute;
}

int16_t usb_audio_in_volume()
{
    return audio_in_volume;
}

int16_t usb_audio_out_volume()
{
    return audio_out_volume;
}

uint32_t usb_audio_in_sample_rate()
{
    return audio_in_sample_rate;
}

uint32_t usb_audio_out_sample_rate()
{
    return audio_out_sample_rate;
}

// -----------------------------------------------------------------------------
// UAC1 Feature Unit GET requests
//
// Handles:
//
// GET_CUR
// GET_MIN
// GET_MAX
// GET_RES
//
// for:
//
// MUTE
// VOLUME
// -----------------------------------------------------------------------------

bool tud_audio_get_req_entity_cb(
    uint8_t rhport,
    tusb_control_request_t const* request)
{
    const uint8_t entity_id =
        static_cast<uint8_t>(request->wIndex >> 8);

    const uint8_t control =
        static_cast<uint8_t>(request->wValue >> 8);

    const uint8_t channel =
        static_cast<uint8_t>(request->wValue & 0xff);

    // We only have one audio channel.
    //
    // channel 0 = master
    // channel 1 = channel 1
    //
    if (channel > AUDIO_CHANNELS)
        return false;

    // -------------------------------------------------------------------------
    // MUTE
    // -------------------------------------------------------------------------

    if (control == AUDIO10_FU_CTRL_MUTE)
    {
        uint8_t value;

        if (entity_id == AUDIO_ENTITY_MIC_FEATURE)
        {
            value = audio_in_mute ? 1 : 0;
        }
        else if (entity_id == AUDIO_ENTITY_SPK_FEATURE)
        {
            value = audio_out_mute ? 1 : 0;
        }
        else
        {
            return false;
        }

        return tud_control_xfer(
            rhport,
            request,
            &value,
            sizeof(value));
    }

    // -------------------------------------------------------------------------
    // VOLUME
    // -------------------------------------------------------------------------

    if (control == AUDIO10_FU_CTRL_VOLUME)
    {
        int16_t value;

        switch (request->bRequest)
        {
            case AUDIO10_CS_REQ_GET_CUR:
            {
                if (entity_id == AUDIO_ENTITY_MIC_FEATURE)
                {
                    value = audio_in_volume;
                }
                else if (entity_id == AUDIO_ENTITY_SPK_FEATURE)
                {
                    value = audio_out_volume;
                }
                else
                {
                    return false;
                }

                break;
            }

            case AUDIO10_CS_REQ_GET_MIN:
                value = AUDIO_VOLUME_MIN;
                break;

            case AUDIO10_CS_REQ_GET_MAX:
                value = AUDIO_VOLUME_MAX;
                break;

            case AUDIO10_CS_REQ_GET_RES:
                value = AUDIO_VOLUME_RES;
                break;

            default:
                return false;
        }

        return tud_control_xfer(
            rhport,
            request,
            &value,
            sizeof(value));
    }

    return false;
}

// -----------------------------------------------------------------------------
// UAC1 Feature Unit SET requests
//
// Handles:
//
// SET_CUR MUTE
// SET_CUR VOLUME
// -----------------------------------------------------------------------------

bool tud_audio_set_req_entity_cb(
    uint8_t rhport,
    tusb_control_request_t const* request,
    uint8_t* buffer)
{
    (void) rhport;

    const uint8_t entity_id =
        static_cast<uint8_t>(request->wIndex >> 8);

    const uint8_t control =
        static_cast<uint8_t>(request->wValue >> 8);

    const uint8_t channel =
        static_cast<uint8_t>(request->wValue & 0xff);

    if (channel > AUDIO_CHANNELS)
        return false;

    // We currently only accept SET_CUR.
    if (request->bRequest != AUDIO10_CS_REQ_SET_CUR)
        return false;

    // -------------------------------------------------------------------------
    // MUTE
    // -------------------------------------------------------------------------

    if (control == AUDIO10_FU_CTRL_MUTE)
    {
        if (request->wLength != 1)
            return false;

        const bool mute = buffer[0] != 0;

        if (entity_id == AUDIO_ENTITY_MIC_FEATURE)
        {
            audio_in_mute = mute;
        }
        else if (entity_id == AUDIO_ENTITY_SPK_FEATURE)
        {
            audio_out_mute = mute;
        }
        else
        {
            return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // VOLUME
    // -------------------------------------------------------------------------

    if (control == AUDIO10_FU_CTRL_VOLUME)
    {
        if (request->wLength != 2)
            return false;

        int16_t volume =
            static_cast<int16_t>(
                static_cast<uint16_t>(buffer[0]) |
                (static_cast<uint16_t>(buffer[1]) << 8));

        volume = std::clamp(
            volume,
            AUDIO_VOLUME_MIN,
            AUDIO_VOLUME_MAX);

        if (entity_id == AUDIO_ENTITY_MIC_FEATURE)
        {
            audio_in_volume = volume;
        }
        else if (entity_id == AUDIO_ENTITY_SPK_FEATURE)
        {
            audio_out_volume = volume;
        }
        else
        {
            return false;
        }

        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// UAC1 endpoint GET requests
//
// Handles GET_CUR sample frequency.
//
// Endpoint:
//
// 0x03 = PC → Pico
// 0x83 = Pico → PC
// -----------------------------------------------------------------------------

bool tud_audio_get_req_ep_cb(
    uint8_t rhport,
    tusb_control_request_t const* request)
{
    const uint8_t control =
        static_cast<uint8_t>(request->wValue >> 8);

    if (control != 0x01)
        return false;

    const uint8_t endpoint =
        static_cast<uint8_t>(request->wIndex & 0xff);

    uint32_t sample_rate;

    if (endpoint == AUDIO_EP_OUT)
    {
        sample_rate = audio_out_sample_rate;
    }
    else if (endpoint == AUDIO_EP_IN)
    {
        sample_rate = audio_in_sample_rate;
    }
    else
    {
        return false;
    }

    // UAC1 sample frequency is 24-bit.
    audio_control_response[0] =
        static_cast<uint8_t>(sample_rate & 0xff);

    audio_control_response[1] =
        static_cast<uint8_t>((sample_rate >> 8) & 0xff);

    audio_control_response[2] =
        static_cast<uint8_t>((sample_rate >> 16) & 0xff);

    return tud_control_xfer(
        rhport,
        request,
        audio_control_response,
        3);
}

// -----------------------------------------------------------------------------
// UAC1 endpoint SET requests
//
// Handles SET_CUR sample frequency.
// -----------------------------------------------------------------------------

bool tud_audio_set_req_ep_cb(
    uint8_t rhport,
    tusb_control_request_t const* request,
    uint8_t* buffer)
{
    (void) rhport;

    const uint8_t control =
        static_cast<uint8_t>(request->wValue >> 8);

    if (control != 0x01)
        return false;

    if (request->bRequest != AUDIO10_CS_REQ_SET_CUR)
        return false;

    if (request->wLength != 3)
        return false;

    const uint32_t sample_rate =
        static_cast<uint32_t>(buffer[0]) |
        (static_cast<uint32_t>(buffer[1]) << 8) |
        (static_cast<uint32_t>(buffer[2]) << 16);

    // Only support our advertised sample rate.
    if (sample_rate != AUDIO_SAMPLE_RATE)
        return false;

    const uint8_t endpoint =
        static_cast<uint8_t>(request->wIndex & 0xff);

    if (endpoint == AUDIO_EP_OUT)
    {
        audio_out_sample_rate = sample_rate;
    }
    else if (endpoint == AUDIO_EP_IN)
    {
        audio_in_sample_rate = sample_rate;
    }
    else
    {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Audio streaming interface callback
//
// Alternate setting:
//
// 0 = stopped
// 1 = streaming
// -----------------------------------------------------------------------------

bool tud_audio_set_itf_cb(
    uint8_t rhport,
    tusb_control_request_t const* request)
{
    (void) rhport;

    const uint8_t interface_number =
        static_cast<uint8_t>(request->wIndex & 0xff);

    const uint8_t alternate_setting =
        static_cast<uint8_t>(request->wValue & 0xff);

    if (interface_number == ITF_NUM_AUDIO_STREAMING_OUT)
    {
        audio_out_streaming =
            alternate_setting != 0;

        return true;
    }

    if (interface_number == ITF_NUM_AUDIO_STREAMING_IN)
    {
        audio_in_streaming =
            alternate_setting != 0;

        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// Optional helper: read PC → Pico audio
// -----------------------------------------------------------------------------

uint16_t usb_audio_read(
    void* buffer,
    uint16_t buffer_size)
{
    if (!audio_out_streaming)
        return 0;

    if (audio_out_mute)
        return 0;

    return tud_audio_read(
        buffer,
        buffer_size);
}

// -----------------------------------------------------------------------------
// Optional helper: write Pico → PC audio
// -----------------------------------------------------------------------------

uint16_t usb_audio_write(
    const void* buffer,
    uint16_t buffer_size)
{
    if (!audio_in_streaming)
        return 0;

    if (audio_in_mute)
        return 0;

    return tud_audio_write(
        buffer,
        buffer_size);
}

// -----------------------------------------------------------------------------
// Debug information
// -----------------------------------------------------------------------------

uint16_t usb_audio_bytes_per_frame()
{
    return AUDIO_BYTES_PER_FRAME;
}