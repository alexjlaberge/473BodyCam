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

#include <usblib/usblib.h>

#include <usblib/host/usbhost.h>

#define UVC_VERSION 0x150

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

struct uvc_cam_term_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t iTerminal;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t bControlSize;
	uint8_t bmControls[3];
};

struct uvc_enc_term_desc
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t iEncoding;
	uint8_t bControlSize;
	uint8_t bmControls[3];
	uint8_t bmControlsRuntime[3];
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

// TODO Video Frame Descriptor, one for each format

#endif /* UVC_H_ */
