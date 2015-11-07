/*
 * uvc.h
 *
 *  Created on: Oct 20, 2015
 *      Author: Alec
 */

#ifndef UVC_H_
#define UVC_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <usblib/usblib.h>

#include <usblib/host/usbhost.h>

#define UVC_VERSION 0x150
#define UVC_SC_UNDEFINED 0x00
#define UVC_SC_VIDEOCONTROL 0x01
#define UVC_SC_VIDEOSTREAMING 0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION 0x03

#define UVC_VC_HEADER 0

/**
 * @brief video class subtypes
 */
#define UVC_DESCRIPTOR_UNDEFINED 0x00
#define UVC_HEADER 0x01
#define UVC_INPUT_TERMINAL 0x02
#define UVC_OUTPUT_TERMINAL 0x03
#define UVC_SELECTOR_UNIT 0x04
#define UVC_PROCESSING_UNIT 0x05
#define UVC_EXTENSION_UNIT 0x06
#define UVC_ENCODING_UNIT 0x07

#define UVC_INPUT_TERMINAL_CAMERA 0x0201

/**
 * @brief some packet lengths
 */
#define UVC_CAM_TERM_SIZE 18
#define UVC_PROC_UNIT_SIZE 13
#define UVC_ENC_UNIT_SIZE 13
#define UVC_ISO_ENDPT_SIZE 7

extern const tUSBHostClassDriver uvc_driver;

/**
 * @brief Video data descriptor
 *
 * @details See Table 2-5 of UVC 1.5 Specification.
 */
struct uvc_payload_header
{
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
	uint32_t dwPresentationTime;
	uint32_t scrSourceClockTime;
	uint16_t scrSourceClockOther;
};

struct uvc_iad
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bFirstInterface;
	uint8_t bInterfaceCount;
	uint8_t bFunctionClass;
	uint8_t bFunctionSubClass;
	uint8_t bFunctionProtocol;
	uint8_t iFunction;
};

struct uvc_ihd
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t bInCollection;
	uint8_t *baInterfaceNr;
};

struct uvc_out_term_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
};

struct uvc_cam_term_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t iTerminal;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t bControlSize;
	uint32_t bmControls;
};

struct uvc_proc_unit_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint16_t wMaxMultiplier;
	uint8_t bControlSize;
	uint32_t bmControls;
	uint8_t iProcessing;
	uint8_t bmVideoStandards;
};

struct uvc_enc_unit_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t iEncoding;
	uint8_t bControlSize;
	uint32_t bmControls;
	uint32_t bmControlsRuntime;
};

struct uvc_stream_ihd
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bNumFormats;
	uint16_t wTotalLength;
	uint8_t bEndpointAddress;
	uint8_t bmInfo;
	uint8_t bTerminalLink;
	uint8_t bStillCaptureMethod;
	uint8_t bTriggerSupport;
	uint8_t bTriggerUsage;
	uint8_t bControlSize;
	uint8_t *bmaControls;
};

struct uvc_iso_endpt_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
};

// TODO Video Frame Descriptor, one for each format

uint8_t uvc_has_error(void);

tConfigDescriptor uvc_get_config(void);

void uvc_iad_init(struct uvc_iad *iad);
struct uvc_iad uvc_get_iad(void);

/**
 * @brief Parse an Interface Association Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_iad(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Video Class Interface Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_vcid(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Class Interface Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_csvcid(uint8_t *buf, size_t max_len);

/**
 * @brief Parse an Input Terminal Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_input_terminal(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Camera Terminal Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_camera_terminal(uint8_t *buf, size_t max_len);

/**
 * @brief Parse an Output Terminal Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_output_terminal(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Selector Unit Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_selector_unit(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Processing Unit Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_processing_unit(uint8_t *buf, size_t max_len);

/**
 * @brief Parse an Encoding Unit Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_encoding_unit(uint8_t *buf, size_t max_len);

/**
 * @brief Parse an Isochronous Endpoint Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_isoc_endpoint(uint8_t *buf, size_t max_len);

/**
 * @brief Parse all the config from the device
 * @param size Size of the entire config
 */
void uvc_parse_all_config(size_t size);

#endif /* UVC_H_ */
