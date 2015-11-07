/*
 * uvc.c
 *
 *  Created on: Oct 21, 2015
 *      Author: Alec
 */

#include "uvc.h"

#include <stdlib.h>

#include <usblib/usblib.h>
#include <usblib/host/usbhost.h>
#include <usblib/host/usbhostpriv.h>

struct usb_pipe
{
	uint32_t pipe;
	uint32_t max_packet_size;
};

#define MAX_ENDPOINTS 4
struct usb_iface
{
	tInterfaceDescriptor iface;
	tEndpointDescriptor endpts[MAX_ENDPOINTS];
};

#define MAX_IFACES 4
struct camera_device
{
	tUSBHostDevice *device;
	uint8_t has_error;
	uint8_t in_use;
	struct usb_pipe intr;
	struct usb_pipe stream;
	struct usb_iface ifaces[MAX_IFACES];
	struct uvc_iad iad;
	uint16_t uvc_version;
	uint8_t nifaces;
	struct uvc_out_term_desc output_terminal;
	struct uvc_cam_term_desc camera_terminal;
	struct uvc_proc_unit_desc proc_unit;
	struct uvc_enc_unit_desc enc_unit;
	struct uvc_iso_endpt_desc iso_endpt;
};

static struct camera_device cam_inst = {0};

/**
 * @brief Called by usblib during device enumeration
 */
static void * uvc_open(tUSBHostDevice *dev);

/**
 * @brief Called by usblib during device shutdown
 */
static void uvc_close(void *dev);

/**
 * TODO document this
 */
static uint32_t uvc_callback(void *camera, uint32_t event, uint32_t msg_param,
		void *msg_data);

/**
 * @brief Callback for pipe status changes
 */
static void uvc_pipe_cb(uint32_t pipe, uint32_t event);

const tUSBHostClassDriver uvc_driver =
{
    USB_CLASS_VIDEO,
    uvc_open,
    uvc_close,
    0
};

static void * uvc_open(tUSBHostDevice *dev)
{
	tInterfaceDescriptor *iface;
	tEndpointDescriptor *endpt;
	uint8_t i;
	uint32_t err = 0;

	iface = USBDescGetInterface(dev->psConfigDescriptor, 0, 0);

	if (!cam_inst.in_use)
	{
		cam_inst.device = dev;
		cam_inst.in_use = 1;

		for (i = 0; i < iface->bNumEndpoints; i++)
		{
			endpt = USBDescGetInterfaceEndpoint(iface, i, 256);

			if (endpt != 0 && endpt->bmAttributes & USB_EP_ATTR_INT)
			{
				cam_inst.intr.pipe = USBHCDPipeAlloc(0, USBHCD_PIPE_INTR_IN,
						dev, uvc_pipe_cb);
				if (cam_inst.intr.pipe == 0)
				{
					goto fail;
				}

				err = USBHCDPipeConfig(cam_inst.intr.pipe, endpt->wMaxPacketSize,
						endpt->bInterval,
						endpt->bEndpointAddress & USB_EP_DESC_NUM_M);
				if (err != 0)
				{
					goto fail;
				}
			}
		}

		return &cam_inst;
	}

fail:
	cam_inst.in_use = 0;
	return (void *) 0;
}

static void uvc_close(void *dev)
{
	/* TODO add USBHCDPipeFree() calls */
	cam_inst.in_use = 0;
	return;
}

static void uvc_pipe_cb(uint32_t pipe, uint32_t event)
{
	if (pipe == cam_inst.intr.pipe)
	{
		if (event == USB_EVENT_ERROR)
		{
			cam_inst.has_error = 1;
		}
	}
}

uint32_t uvc_callback(void *camera, uint32_t event, uint32_t msg_param,
		void *msg_data)
{
	return 0;
}

#define BUF_SIZE 768
tConfigDescriptor uvc_get_config(void)
{
	tConfigDescriptor conf = {0};
	uint8_t all[BUF_SIZE];
	tUSBRequest req;
	uint32_t sent;
	size_t i = 0;
	size_t all_i = 0;

	if (!cam_inst.in_use)
	{
		return conf;
	}

	req.bmRequestType = USB_RTYPE_DIR_IN | USB_RTYPE_STANDARD |
			USB_RTYPE_DEVICE;
	req.bRequest = USBREQ_GET_DESCRIPTOR;
	req.wValue = USB_DTYPE_CONFIGURATION << 8;
	req.wIndex = 0;
	req.wLength = sizeof(tConfigDescriptor);

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &conf,
			sizeof(tConfigDescriptor),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	if (conf.wTotalLength <= BUF_SIZE)
	{
		uvc_parse_all_config(conf.wTotalLength);
	}

	return conf;
}

void uvc_parse_all_config(size_t size)
{
	tUSBRequest req;
	uint8_t buf[BUF_SIZE];
	uint32_t len;
	size_t i, iface, endpt;
	size_t procd;

	req.bmRequestType = USB_RTYPE_DIR_IN | USB_RTYPE_STANDARD |
			USB_RTYPE_DEVICE;
	req.bRequest = USBREQ_GET_DESCRIPTOR;
	req.wValue = (USB_DTYPE_CONFIGURATION << 8) | 0;
	req.wIndex = 0;
	req.wLength = size;

	len = USBHCDControlTransfer(0, &req, cam_inst.device, buf,
		BUF_SIZE, cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	if (len < sizeof(tConfigDescriptor))
	{
		return;
	}

	i = sizeof(tConfigDescriptor);
	while (i < len)
	{
		struct usb_iface tmp;
		uint8_t desc_len;
		uint8_t desc_type;

		// The first byte in every descriptor is the length
		desc_len = buf[i];
		desc_type = buf[i + 1];
		procd = 0;

		switch (desc_type)
		{
		case USB_DTYPE_CONFIGURATION:
			procd = buf[i];
			break;
		case USB_DTYPE_CS_INTERFACE:
			procd = uvc_parse_csvcid(buf + i, len - i);
			break;
		case USB_DTYPE_DEVICE:
			procd = buf[i];
			break;
		case USB_DTYPE_DEVICE_QUAL:
			procd = buf[i];
			break;
		case USB_DTYPE_ENDPOINT:
			procd = uvc_parse_isoc_endpoint(buf + i, len - i);
			break;
		case USB_DTYPE_HUB:
			procd = buf[i];
			break;
		case USB_DTYPE_INTERFACE:
			procd = uvc_parse_vcid(buf + i, len - i);
			break;
		case USB_DTYPE_INTERFACE_ASC:
			procd = uvc_parse_iad(buf + i, len - i);
			break;
		case USB_DTYPE_INTERFACE_PWR:
			procd = buf[i];
			break;
		case USB_DTYPE_OSPEED_CONF:
			procd = buf[i];
			break;
		case USB_DTYPE_OTG:
			procd = buf[i];
			break;
		case USB_DTYPE_STRING:
			procd = buf[i];
			break;
		default:
			break;
		}

		if (procd == 0)
		{
			return;
		}

		i += procd;
	}
}

uvc_enc_unit_desc_init(struct uvc_enc_unit_desc *desc)
{
	size_t i = 0;

	desc->bControlSize = 0;
	desc->bDescriptorSubtype = 0;
	desc->bDescriptorType = 0;
	desc->bUnitID = 0;
	desc->bSourceID = 0;
	desc->iEncoding = 0;
	desc->bControlSize = 0;
	desc->bmControls = 0;
	desc->bmControlsRuntime = 0;
}

struct uvc_iad uvc_get_iad(void)
{
	struct uvc_iad ret = {0};
	tUSBRequest req;
	uint32_t sent;

	if (!cam_inst.in_use)
	{
		return ret;
	}

	uvc_iad_init(&ret);

	req.bmRequestType = USB_RTYPE_DIR_IN | USB_RTYPE_STANDARD |
            USB_RTYPE_DEVICE;
	req.bRequest = USBREQ_GET_DESCRIPTOR;
	req.wValue = USB_DTYPE_INTERFACE_ASC << 8;
	req.wIndex = 0;
	req.wLength = sizeof(struct uvc_iad);

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &ret,
			sizeof(struct uvc_iad),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);
	if (sent != sizeof(struct uvc_iad))
	{
		goto fail;
	}

	return ret;

fail:
	uvc_iad_init(&ret);
	return ret;
}

void uvc_iad_init(struct uvc_iad *iad)
{
	iad->bLength = 0;
	iad->bDescriptorType = 0;
	iad->bFirstInterface = 0;
	iad->bInterfaceCount = 0;
	iad->bFunctionClass = 0;
	iad->bFunctionSubClass = 0;
	iad->bFunctionProtocol = 0;
	iad->iFunction = 0;
}

uint8_t uvc_has_error(void)
{
	return cam_inst.has_error;
}

size_t uvc_parse_iad(uint8_t *buf, size_t max_len)
{
	if (sizeof(struct uvc_iad) <= max_len &&
		buf[0] == sizeof(struct uvc_iad) &&
		buf[1] == USB_DTYPE_INTERFACE_ASC)
	{
		memcpy((uint8_t *) &cam_inst.iad, buf, sizeof(struct uvc_iad));

		if (cam_inst.iad.bFunctionClass == USB_CLASS_VIDEO &&
			cam_inst.iad.bFunctionSubClass == UVC_SC_VIDEO_INTERFACE_COLLECTION)
		{
			return sizeof(struct uvc_iad);
		}
	}

	return 0;
}

size_t uvc_parse_vcid(uint8_t *buf, size_t max_len)
{
	tInterfaceDescriptor tmp;

	if (sizeof(tInterfaceDescriptor) <= max_len &&
		buf[0] == sizeof(tInterfaceDescriptor) &&
		buf[1] == USB_DTYPE_INTERFACE)
	{
		memcpy(&tmp, buf, sizeof(tInterfaceDescriptor));

		if (tmp.bInterfaceClass == USB_CLASS_VIDEO &&
			tmp.bInterfaceSubClass == UVC_SC_VIDEOCONTROL)
		{
			return sizeof(tInterfaceDescriptor);
		}
	}

	return 0;
}

size_t uvc_parse_csvcid(uint8_t *buf, size_t max_len)
{
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint16_t total_len;
	size_t i;

	if (max_len < 12)
	{
		return 0;
	}

	len = buf[0];
	type = buf[1];
	subtype = buf[2];
	if (len < 12 ||
		type != USB_DTYPE_CS_INTERFACE ||
		subtype != UVC_HEADER)
	{
		return 0;
	}

	uint16_t *tmp = (uint16_t *) (buf + 3);
	cam_inst.uvc_version = *tmp;
	if (cam_inst.uvc_version != UVC_VERSION)
	{
		//return 0;
	}

	tmp = (uint16_t *) (buf + 5);
	total_len = *tmp;

	cam_inst.nifaces = buf[11];

	i = len;
	while (i < total_len && i < max_len)
	{
		len = 0;
		type = buf[i + 1];
		subtype = buf[i + 2];

		if (type != USB_DTYPE_CS_INTERFACE)
		{
			break;
		}

		switch (subtype)
		{
		case UVC_ENCODING_UNIT:
			len = uvc_parse_encoding_unit(buf + i, max_len - i);
			break;
		case UVC_INPUT_TERMINAL:
			len = uvc_parse_input_terminal(buf + i, max_len - i);
			break;
		case UVC_OUTPUT_TERMINAL:
			len = uvc_parse_output_terminal(buf + i, max_len - i);
			break;
		case UVC_PROCESSING_UNIT:
			len = uvc_parse_processing_unit(buf + i, max_len - i);
			break;
		case UVC_SELECTOR_UNIT:
			len = uvc_parse_selector_unit(buf + i, max_len - i);
			break;
		case UVC_EXTENSION_UNIT:
			len = uvc_parse_extension_unit(buf + i, max_len - i);
			break;
		default:
			break;
		}

		if (len == 0)
		{
			break;
		}

		i += len;
	}

	return i;
}

#define UVC_INPUT_TERM_MIN_LENGTH 8
size_t uvc_parse_input_terminal(uint8_t *buf, size_t max_len)
{
	uint8_t len;
	uint16_t *tmp;

	if (max_len < UVC_INPUT_TERM_MIN_LENGTH ||
		buf[0] < UVC_INPUT_TERM_MIN_LENGTH)
	{
		return 0;
	}

	tmp = (uint16_t *) (buf + 4);
	if (*tmp == UVC_INPUT_TERMINAL_CAMERA)
	{
		return uvc_parse_camera_terminal(buf, max_len);
	}

	len = buf[0];

	return len;
}

#define UVC_OUT_TERM_MIN_LENGTH 9
size_t uvc_parse_output_terminal(uint8_t *buf, size_t max_len)
{
	uint16_t *tmp;
	struct uvc_out_term_desc term;

	if (max_len < UVC_OUT_TERM_MIN_LENGTH)
	{
		return 0;
	}

	term.bLength = buf[0];
	term.bDescriptorType = buf[1];
	term.bDescriptorSubtype = buf[2];
	term.bTerminalID = buf[3];

	tmp = (uint16_t *) (buf + 4);

	term.wTerminalType = *tmp;
	term.bAssocTerminal = buf[6];
	term.bSourceID = buf[7];
	term.iTerminal = buf[8];

	if (term.bLength < UVC_OUT_TERM_MIN_LENGTH ||
		term.bDescriptorType != USB_DTYPE_CS_INTERFACE ||
		term.bDescriptorSubtype != UVC_OUTPUT_TERMINAL)
	{
		return 0;
	}

	cam_inst.output_terminal = term;
	return term.bLength;
}

size_t uvc_parse_camera_terminal(uint8_t *buf, size_t max_len)
{
	struct uvc_cam_term_desc term;
	uint16_t *tmp;
	uint32_t *tmp32;

	if (max_len < UVC_CAM_TERM_SIZE)
	{
		return 0;
	}

	term.bLength = buf[0];
	term.bDescriptorType = buf[1];
	term.bDescriptorSubtype = buf[2];
	term.bTerminalID = buf[3];

	tmp = (uint16_t *) (buf + 4);
	term.wTerminalType = *tmp;

	term.bAssocTerminal = buf[6];
	term.iTerminal = buf[7];

	tmp = (uint16_t *) (buf + 8);
	term.wObjectiveFocalLengthMin = *tmp;

	tmp = (uint16_t *) (buf + 10);
	term.wObjectiveFocalLengthMax = *tmp;

	tmp = (uint16_t *) (buf + 12);
	term.wOcularFocalLength = *tmp;

	term.bControlSize = buf[14];

	tmp32 = (uint32_t *) (buf + 15);
	term.bmControls = *tmp & 0x00FFFFFF;

	if (term.bLength != UVC_CAM_TERM_SIZE ||
		term.bDescriptorType != USB_DTYPE_CS_INTERFACE ||
		term.bDescriptorSubtype != UVC_INPUT_TERMINAL)
	{
		return 0;
	}

	cam_inst.camera_terminal = term;
	return UVC_CAM_TERM_SIZE;
}

size_t uvc_parse_selector_unit(uint8_t *buf, size_t max_len)
{
	// TODO this is basically a no-op
	return buf[0];
}

size_t uvc_parse_processing_unit(uint8_t *buf, size_t max_len)
{
	uint16_t *tmp;
	uint32_t *ctrl;
	size_t len;

	if (max_len < UVC_PROC_UNIT_SIZE)
	{
		return 0;
	}

	len = buf[0];
	if (buf[1] != USB_DTYPE_CS_INTERFACE ||
		buf[2] != UVC_PROCESSING_UNIT)
	{
		return 0;
	}

	cam_inst.proc_unit.bLength = buf[0];
	cam_inst.proc_unit.bDescriptorType = buf[1];
	cam_inst.proc_unit.bDescriptorSubtype = buf[2];
	cam_inst.proc_unit.bUnitID = buf[3];
	cam_inst.proc_unit.bSourceID = buf[4];

	tmp = (uint16_t *) (buf + 5);
	cam_inst.proc_unit.wMaxMultiplier = *tmp;

	cam_inst.proc_unit.bControlSize = buf[7];

	ctrl = (uint32_t *) (buf + 8);
	cam_inst.proc_unit.bmControls = *ctrl & 0x00FFFFFF;
	cam_inst.proc_unit.iProcessing = buf[11];
	cam_inst.proc_unit.bmVideoStandards = buf[12];

	return len;
}

size_t uvc_parse_encoding_unit(uint8_t *buf, size_t max_len)
{
	uint32_t *tmp;

	if (max_len < UVC_ENC_UNIT_SIZE)
	{
		return 0;
	}

	if (buf[0] != UVC_ENC_UNIT_SIZE ||
		buf[1] != USB_DTYPE_CS_INTERFACE ||
		buf[2] != UVC_ENCODING_UNIT)
	{
		return 0;
	}

	cam_inst.enc_unit.bLength = buf[0];
	cam_inst.enc_unit.bDescriptorType = buf[1];
	cam_inst.enc_unit.bDescriptorSubtype = buf[2];

	cam_inst.enc_unit.bUnitID = buf[3];
	cam_inst.enc_unit.bSourceID = buf[4];
	cam_inst.enc_unit.iEncoding = buf[5];
	cam_inst.enc_unit.bControlSize = buf[6];

	tmp = (uint32_t *) (buf + 7);
	cam_inst.enc_unit.bmControls = *tmp & 0x00FFFFFF;

	tmp = (uint32_t *) (buf + 10);
	cam_inst.enc_unit.bmControlsRuntime = *tmp & 0x00FFFFFF;

	return UVC_ENC_UNIT_SIZE;
}

size_t uvc_parse_isoc_endpoint(uint8_t *buf, size_t max_len)
{
	struct uvc_iso_endpt_desc endpt;
	uint16_t *tmp;

	if (max_len < UVC_ISO_ENDPT_SIZE)
	{
		return 0;
	}

	endpt.bLength = buf[0];
	endpt.bDescriptorType = buf[1];
	endpt.bEndpointAddress = buf[2];
	endpt.bmAttributes = buf[3];

	tmp = (uint16_t *) (buf + 4);
	endpt.wMaxPacketSize = *tmp;

	endpt.bInterval = buf[6];

	if (endpt.bDescriptorType == USB_DTYPE_ENDPOINT &&
		(endpt.bEndpointAddress & USB_RTYPE_DIR_IN) &&
		(endpt.bmAttributes & 0x03))
	{
		cam_inst.iso_endpt = endpt;
		return UVC_ISO_ENDPT_SIZE;
	}

	return 0;
}

size_t uvc_parse_extension_unit(uint8_t *buf, size_t max_len)
{
	return buf[0];
}
