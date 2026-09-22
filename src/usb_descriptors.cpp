#include "tusb.h"
#include <cstring>

// ====================================================================
// 1. DEVICE DESCRIPTOR
// ====================================================================
tusb_desc_device_t const desc_device = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,

	.bDeviceClass = TUSB_CLASS_MISC,
	.bDeviceSubClass = MISC_SUBCLASS_COMMON,
	.bDeviceProtocol = MISC_PROTOCOL_IAD,

	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

	.idVendor = 0xCAFE,
	.idProduct = 0x4001,
	.bcdDevice = 0x0100,

	.iManufacturer = 0x01,
	.iProduct = 0x02,
	.iSerialNumber = 0x03,

	.bNumConfigurations = 0x01};

extern "C" uint8_t const *tud_descriptor_device_cb(void) {
	return (uint8_t const *)&desc_device;
}

// ====================================================================
// 2. CONFIGURATION DESCRIPTOR
// ====================================================================
enum {
	ITF_NUM_CDC_0 = 0,
	ITF_NUM_CDC_0_DATA,
	ITF_NUM_AUDIO_CONTROL,
	ITF_NUM_AUDIO_STREAMING_OUT, // Host -> Pico (Radio TX audio, speaker path)
	ITF_NUM_AUDIO_STREAMING_IN,	 // Pico -> Host (Radio RX audio, mic path)
	ITF_NUM_TOTAL
};

#define EPNUM_CDC_0_NOTIF 0x81
#define EPNUM_CDC_0_OUT 0x02
#define EPNUM_CDC_0_IN 0x82

#define EPNUM_AUDIO_OUT 0x03 // speaker path, OUT
#define EPNUM_AUDIO_IN 0x83	 // mic path, IN

// Entity IDs
// Mic path (radio -> PC): Input Term 1 -> Feature Unit 2 -> Output Term 3 (USB streaming)
// Speaker path (PC -> radio): Input Term 4 (USB streaming) -> Feature Unit 5 -> Output Term 6
#define UAC1_MIC_IT 0x01
#define UAC1_MIC_FU 0x02
#define UAC1_MIC_OT 0x03
#define UAC1_SPK_IT 0x04
#define UAC1_SPK_FU 0x05
#define UAC1_SPK_OT 0x06

// TinyUSB's UAC1 templates don't include an IAD macro (only UAC2 does) -
// build one ourselves so the 3-interface audio function enumerates as a
// single grouped function under Windows.
#define TUD_AUDIO10_DESC_IAD_LEN 8
#define TUD_AUDIO10_DESC_IAD(_firstitf, _nitfs, _stridx)                          \
	TUD_AUDIO10_DESC_IAD_LEN, TUSB_DESC_INTERFACE_ASSOCIATION, _firstitf, _nitfs, \
		TUSB_CLASS_AUDIO, AUDIO_FUNCTION_SUBCLASS_UNDEFINED, AUDIO_FUNC_PROTOCOL_CODE_UNDEF, _stridx

// One supported sample rate per streaming interface -> _nfreqs = 1
#define CONFIG_TOTAL_LEN (                                                                                           \
	TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN +                                                                         \
	TUD_AUDIO10_DESC_IAD_LEN +                                                                                       \
	TUD_AUDIO10_DESC_STD_AC_LEN + TUD_AUDIO10_DESC_CS_AC_LEN(2) +                                                    \
	TUD_AUDIO10_DESC_INPUT_TERM_LEN + TUD_AUDIO10_DESC_FEATURE_UNIT_LEN(1) + TUD_AUDIO10_DESC_OUTPUT_TERM_LEN +      \
	TUD_AUDIO10_DESC_INPUT_TERM_LEN + TUD_AUDIO10_DESC_FEATURE_UNIT_LEN(1) + TUD_AUDIO10_DESC_OUTPUT_TERM_LEN +      \
	2 * TUD_AUDIO10_DESC_STD_AS_LEN + TUD_AUDIO10_DESC_CS_AS_INT_LEN +                                               \
	TUD_AUDIO10_DESC_TYPE_I_FORMAT_LEN(1) + TUD_AUDIO10_DESC_STD_AS_ISO_EP_LEN + TUD_AUDIO10_DESC_CS_AS_ISO_EP_LEN + \
	2 * TUD_AUDIO10_DESC_STD_AS_LEN + TUD_AUDIO10_DESC_CS_AS_INT_LEN +                                               \
	TUD_AUDIO10_DESC_TYPE_I_FORMAT_LEN(1) + TUD_AUDIO10_DESC_STD_AS_ISO_EP_LEN + TUD_AUDIO10_DESC_CS_AS_ISO_EP_LEN)

uint8_t const desc_configuration[] = {
	// Configuration Header
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

	// Virtual COM Port (CDC Interface)
	TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),

	// --- Audio function: IAD + Control interface + 2 Streaming interfaces ---
	TUD_AUDIO10_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 3, 0),

	TUD_AUDIO10_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0x00, 0),
	TUD_AUDIO10_DESC_CS_AC(
		/*_bcdADC*/ 0x0100,
		/*_totallen*/ (TUD_AUDIO10_DESC_INPUT_TERM_LEN + TUD_AUDIO10_DESC_FEATURE_UNIT_LEN(1) + TUD_AUDIO10_DESC_OUTPUT_TERM_LEN) * 2,
		ITF_NUM_AUDIO_STREAMING_OUT, ITF_NUM_AUDIO_STREAMING_IN),

	// Mic path (radio -> PC)
	TUD_AUDIO10_DESC_INPUT_TERM(UAC1_MIC_IT, AUDIO_TERM_TYPE_IN_GENERIC_MIC, 0x00, 1, AUDIO10_CHANNEL_CONFIG_NON_PREDEFINED, 0, 5),
	TUD_AUDIO10_DESC_FEATURE_UNIT(UAC1_MIC_FU, UAC1_MIC_IT, 0,
								  (AUDIO10_FU_CONTROL_BM_MUTE | AUDIO10_FU_CONTROL_BM_VOLUME),
								  (AUDIO10_FU_CONTROL_BM_MUTE | AUDIO10_FU_CONTROL_BM_VOLUME)),
	TUD_AUDIO10_DESC_OUTPUT_TERM(UAC1_MIC_OT, AUDIO_TERM_TYPE_USB_STREAMING, 0x00, UAC1_MIC_FU, 0),

	// Speaker path (PC -> radio)
	TUD_AUDIO10_DESC_INPUT_TERM(UAC1_SPK_IT, AUDIO_TERM_TYPE_USB_STREAMING, 0x00, 1, AUDIO10_CHANNEL_CONFIG_NON_PREDEFINED, 0, 6),
	TUD_AUDIO10_DESC_FEATURE_UNIT(UAC1_SPK_FU, UAC1_SPK_IT, 0,
								  (AUDIO10_FU_CONTROL_BM_MUTE | AUDIO10_FU_CONTROL_BM_VOLUME),
								  (AUDIO10_FU_CONTROL_BM_MUTE | AUDIO10_FU_CONTROL_BM_VOLUME)),
	TUD_AUDIO10_DESC_OUTPUT_TERM(UAC1_SPK_OT, AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER, 0x00, UAC1_SPK_FU, 0),

	// Streaming interface: speaker (OUT, host -> device)
	TUD_AUDIO10_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_OUT, 0x00, 0x00, 6),
	TUD_AUDIO10_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_OUT, 0x01, 0x01, 6),
	TUD_AUDIO10_DESC_CS_AS_INT(UAC1_SPK_IT, 0x01, AUDIO10_DATA_FORMAT_TYPE_I_PCM),
	TUD_AUDIO10_DESC_TYPE_I_FORMAT(1, 2, 16, 48000),
	TUD_AUDIO10_DESC_STD_AS_ISO_EP(EPNUM_AUDIO_OUT,
								   (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE), 98, 0x01, 0x00),
	TUD_AUDIO10_DESC_CS_AS_ISO_EP(AUDIO10_CS_AS_ISO_DATA_EP_ATT_SAMPLING_FRQ,
								  AUDIO10_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, 0x0001),

	// Streaming interface: mic (IN, device -> host)
	TUD_AUDIO10_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_IN, 0x00, 0x00, 5),
	TUD_AUDIO10_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_IN, 0x01, 0x01, 5),
	TUD_AUDIO10_DESC_CS_AS_INT(UAC1_MIC_OT, 0x01, AUDIO10_DATA_FORMAT_TYPE_I_PCM),
	TUD_AUDIO10_DESC_TYPE_I_FORMAT(1, 2, 16, 48000),
	TUD_AUDIO10_DESC_STD_AS_ISO_EP(EPNUM_AUDIO_IN,
								   (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS), 98, 0x01, 0x00),
	TUD_AUDIO10_DESC_CS_AS_ISO_EP(AUDIO10_CS_AS_ISO_DATA_EP_ATT_SAMPLING_FRQ,
								  AUDIO10_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, 0x0001),
};

extern "C" uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
	(void)index;
	return desc_configuration;
}

// ====================================================================
// 3. STRING DESCRIPTORS
// ====================================================================
char const *string_desc_arr[] = {
	(const char[]){0x09, 0x04},
	"Jeremy",
	"Pico-Rig Sound Card",
	"123456",
	"Pico-Rig Serial Port",
	"Pico-Rig Radio Audio In",
	"Pico-Rig Radio Audio Out"};

static uint16_t _desc_str[32]; // buffer, not a scalar

extern "C" uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
	(void)langid;
	uint8_t chr_count;

	if (index == 0) {
		memcpy(&_desc_str[0], string_desc_arr[0], 2);
		chr_count = 1;
	} else {
		if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
		const char *str = string_desc_arr[index];
		chr_count = strlen(str);
		if (chr_count > 31) chr_count = 31;
		for (uint8_t i = 0; i < chr_count; i++) {
			_desc_str[1 + i] = str[i];
		}
	}

	_desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * (chr_count + 1));
	return _desc_str;
}
