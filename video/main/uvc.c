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

#define INT_BUFFER_SIZE 64
#define VIDEO_BUFFER_SIZE 256

struct usb_pipe
{
	uint8_t in_use;
	uint32_t pipe;
	uint32_t endpt;
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
	uint8_t nformats;
	struct uvc_out_term_desc output_terminal;
	struct uvc_cam_term_desc camera_terminal;
	struct uvc_proc_unit_desc proc_unit;
	struct uvc_enc_unit_desc enc_unit;
	struct uvc_iso_endpt_desc iso_endpt;
	uint32_t frame_interval;
	uint8_t streaming_iface;
	uint8_t streaming_endpt;
	uint8_t streaming_alt;
	uint8_t uncomp_format_idx;
	uint8_t uncomp_frame_idx;
};

static struct camera_device cam_inst = {0};
uint8_t cam_stream_buf[VIDEO_BUFFER_SIZE];
uint8_t int_stream_buf[INT_BUFFER_SIZE];

/**
 * @brief Called by usblib during device enumeration
 */
static void * uvc_open(tUSBHostDevice *dev);

/**
 * @brief Called by usblib during device shutdown
 */
static void uvc_close(void *dev);

/**
 * @brief Callback for pipe status changes
 */
static void uvc_pipe_cb(uint32_t pipe, uint32_t event);

uint32_t uvc_probe_set_cur_10(void);
uint32_t uvc_probe_set_cur_15(void);

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
	size_t j;
	uint32_t err = 0;

	for (j = 0; j < VIDEO_BUFFER_SIZE; j++)
	{
		cam_stream_buf[j] = 0;
	}

	for (j = 0; j < INT_BUFFER_SIZE; j++)
	{
		int_stream_buf[j] = 0;
	}

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
	uint32_t len;
	uint8_t pkt_len;
	uint8_t pkt_type;
	uint8_t pkt_subtype;

	if (pipe == cam_inst.intr.pipe)
	{
		switch (event)
		{
		case USB_EVENT_ERROR:
			cam_inst.has_error = 1;
			break;
		case USB_EVENT_SCHEDULER:
			len = USBHCDPipeSchedule(pipe, int_stream_buf, INT_BUFFER_SIZE);
			break;
		case USB_EVENT_RX_AVAILABLE:
			len = USBHCDPipeTransferSizeGet(pipe);
			pkt_len = int_stream_buf[0];
			pkt_type = int_stream_buf[1];
			pkt_subtype = int_stream_buf[2];
			USBHCDPipeDataAck(pipe);
			break;
		case USB_EVENT_STALL:
			uvc_parsing_fault(20);
			break;
		default:
			uvc_parsing_fault(event);
			break;
		}
	}
	else if (cam_inst.stream.in_use && pipe == cam_inst.stream.pipe)
	{
		switch (event)
		{
		case USB_EVENT_SCHEDULER:
			len = USBHCDPipeSchedule(pipe, cam_stream_buf, VIDEO_BUFFER_SIZE);
			break;
		case USB_EVENT_RX_AVAILABLE:
			len = USBHCDPipeTransferSizeGet(pipe);
			pkt_len = cam_stream_buf[0];
			pkt_type = cam_stream_buf[1];
			pkt_subtype = cam_stream_buf[2];
			USBHCDPipeDataAck(pipe);
			break;
		default:
			uvc_parsing_fault(event);
			break;
		}
	}
	else
	{
		uvc_parsing_fault(15);
	}
}

void uvc_main(void)
{
	USBHCDPipeSchedule(cam_inst.stream.pipe, cam_stream_buf,
		VIDEO_BUFFER_SIZE);
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

	// jump past the standard configuration descriptor
	i = buf[0];

	// parse control interfaces
	i += uvc_parse_control(buf + i, len - i);

	// parse streaming interfaces
	while (i < len)
	{
		procd = 0;
		uint8_t type = buf[i + 1];

		if (type == USB_DTYPE_INTERFACE)
		{
			procd = uvc_parse_streaming(buf + i, len - i);
		}
		else
		{
			// done with the video parts of the interface
			break;
		}

		if (procd == 0)
		{
			uvc_parsing_fault(0);
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
		subtype != UVC_VC_HEADER)
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
		case UVC_VC_ENCODING_UNIT:
			len = uvc_parse_encoding_unit(buf + i, max_len - i);
			break;
		case UVC_VC_INPUT_TERMINAL:
			len = uvc_parse_input_terminal(buf + i, max_len - i);
			break;
		case UVC_VC_OUTPUT_TERMINAL:
			len = uvc_parse_output_terminal(buf + i, max_len - i);
			break;
		case UVC_VC_PROCESSING_UNIT:
			len = uvc_parse_processing_unit(buf + i, max_len - i);
			break;
		case UVC_VC_SELECTOR_UNIT:
			len = uvc_parse_selector_unit(buf + i, max_len - i);
			break;
		case UVC_VC_EXTENSION_UNIT:
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
	if (*tmp == UVC_ITT_CAMERA)
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
		term.bDescriptorSubtype != UVC_VC_OUTPUT_TERMINAL)
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
		term.bDescriptorSubtype != UVC_VC_INPUT_TERMINAL)
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
		buf[2] != UVC_VC_PROCESSING_UNIT)
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
		buf[2] != UVC_VC_ENCODING_UNIT)
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

void uvc_parsing_fault(uint8_t i)
{
	while (1)
	{
	}
}

#define UVC_CS_VS_INPUT_HEADER_MIN_SIZE 13
size_t uvc_parse_cs_vs_input_header(uint8_t *buf, size_t max_len)
{
	size_t len;
	uint8_t type;
	uint8_t subtype;
	uint16_t *total_length;
	uint16_t i;

	len = buf[0];
	if (len > max_len)
	{
		uvc_parsing_fault(0);
	}

	if (len < UVC_CS_VS_INPUT_HEADER_MIN_SIZE)
	{
		uvc_parsing_fault(1);
	}

	type = buf[1];
	if (type != USB_DTYPE_CS_INTERFACE)
	{
		uvc_parsing_fault(2);
	}

	subtype = buf[2];
	if (subtype != UVC_VS_INPUT_HEADER)
	{
		uvc_parsing_fault(3);
	}

	cam_inst.nformats = buf[3];
	total_length = (uint16_t *) (buf + 4);
	cam_inst.streaming_endpt = buf[6];

	i = len;
	while (i < *total_length)
	{
		len = 0;
		type = buf[i + 1];
		subtype = buf[i + 2];

		if (type == USB_DTYPE_CS_INTERFACE)
		{
			if (i == buf[0] && buf[i] >= 24)
			{
				uint32_t *tmp = (uint32_t *) (buf + i + 21);
				//cam_inst.frame_interval = *tmp;
			}

			switch (subtype)
			{
			case UVC_VS_FORMAT_MJPEG:
				len = uvc_parse_mjpeg_format_desc(buf + i, max_len - i);
				break;
			case UVC_VS_FRAME_MJPEG:
				len = uvc_parse_mjpeg_frame_desc(buf + i, max_len - i);
				break;
			case UVC_VS_FORMAT_H264:
				len = uvc_parse_h264_format_desc(buf + i, max_len - i);
				break;
			case UVC_VS_FRAME_H264:
				len = uvc_parse_h264_frame_desc(buf + i, max_len - i);
				break;
			case UVC_VS_FORMAT_UNCOMPRESSED:
				len = uvc_parse_uncomp_format_desc(buf + i, max_len - i);
				break;
			default:
				len = buf[i];
				break;
			}
		}
		else
		{
			break;
		}

		if (len == 0)
		{
			uvc_parsing_fault(4);
		}

		i += len;
	}

	return i;
}

size_t uvc_parse_mjpeg_format_desc(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t subtype;

	if (len > max_len)
	{
		return 0;
	}

	/* TODO this will probably require further implementation */

	return len;
}

size_t uvc_parse_mjpeg_frame_desc(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint16_t *width;
	uint16_t *height;
	uint8_t index;
	uint8_t subtype;

	if (len > max_len)
	{
		return 0;
	}

	subtype = buf[2];

	if (subtype != UVC_VS_FRAME_MJPEG)
	{
		uvc_parsing_fault(0);
	}

	index = buf[3];
	width = (uint16_t *) (buf + 5);
	height = (uint16_t *) (buf + 7);

	return len;
}

size_t uvc_parse_h264_format_desc(uint8_t *buf, size_t max_len)
{
	return buf[0];
}

size_t uvc_parse_h264_frame_desc(uint8_t *buf, size_t max_len)
{
	return buf[0];
}

size_t uvc_parse_control(uint8_t *buf, size_t max_len)
{
	size_t parsed = 0;

	parsed += uvc_parse_iad(buf + parsed, max_len - parsed);
	parsed += uvc_parse_vcid(buf + parsed, max_len - parsed);
	parsed += uvc_parse_csvcid(buf + parsed, max_len - parsed);

	while (parsed < max_len)
	{
		if (buf[parsed + 1] == USB_DTYPE_ENDPOINT)
		{
			parsed += uvc_parse_ctrl_endpoint(buf + parsed, max_len - parsed);
		}
		else if (buf[parsed + 1] == USB_DTYPE_CS_ENDPOINT)
		{
			parsed += buf[parsed];
		}
		else
		{
			break;
		}
	}

	return parsed;
}

size_t uvc_parse_streaming(uint8_t *buf, size_t max_len)
{
	size_t parsed = 0;

	parsed += uvc_parse_vsid(buf + parsed, max_len - parsed);

	if (buf[parsed + 1] == USB_DTYPE_CS_INTERFACE)
	{
		parsed += uvc_parse_cs_vs_input_header(buf + parsed, max_len - parsed);
	}

	while (parsed < max_len)
	{
		if (buf[parsed + 1] == USB_DTYPE_ENDPOINT)
		{
			parsed += uvc_parse_streaming_endpoint(buf + parsed, max_len - parsed);
		}
		else if (buf[parsed + 1] == USB_DTYPE_CS_ENDPOINT)
		{
			parsed += buf[parsed];
		}
		else
		{
			break;
		}
	}

	return parsed;
}

size_t uvc_parse_vsid(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t type = buf[1];
	uint8_t class = buf[5];
	uint8_t sc = buf[6];
	uint8_t alternate_setting = buf[3];

	if (type != USB_DTYPE_INTERFACE)
	{
		uvc_parsing_fault(0);
	}

	if (class != USB_CLASS_VIDEO)
	{
		uvc_parsing_fault(1);
	}

	if (sc != UVC_SC_VIDEOSTREAMING)
	{
		uvc_parsing_fault(2);
	}

	if (!cam_inst.stream.in_use)
	{
		cam_inst.streaming_alt = alternate_setting;
		cam_inst.streaming_iface = buf[2];
	}

	return len;
}

size_t uvc_parse_ctrl_endpoint(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t type = buf[1];

	if (type != USB_DTYPE_ENDPOINT)
	{
		uvc_parsing_fault(0);
	}

	return len;
}

size_t uvc_parse_streaming_endpoint(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t type = buf[1];
	uint8_t addr = buf[2];
	uint8_t attr = buf[3];
	uint16_t *max_packet_size = (uint16_t *) (buf + 4);
	uint8_t interval = buf[6];
	uint8_t err;

	if (type != USB_DTYPE_ENDPOINT)
	{
		uvc_parsing_fault(0);
	}

	if (!cam_inst.stream.in_use)
	{
		cam_inst.stream.endpt = addr & 0x0F;
		cam_inst.stream.max_packet_size = *max_packet_size;

		cam_inst.stream.pipe = USBHCDPipeAllocSize(0, USBHCD_PIPE_ISOC_IN_DMA,
				cam_inst.device, *max_packet_size, uvc_pipe_cb);

		err = USBHCDPipeConfig(cam_inst.stream.pipe, *max_packet_size,
				interval,
				cam_inst.stream.endpt);

		cam_inst.stream.in_use = 1;
	}

	return len;
}

// TODO 1. vc_probe_control(set_cur) -> returns vc_probe_control(get_cur)?
// TODO 2. vc_commit_control(set_cur)
// TODO 3. set_interface(1)
uint32_t uvc_probe_set_cur(void)
{
	switch (cam_inst.uvc_version)
	{
	case 0x100:
		return uvc_probe_set_cur_10();
	case 0x150:
		return uvc_probe_set_cur_15();
	default:
		return uvc_probe_set_cur_15();
	}
}

uint32_t uvc_probe_set_cur_10(void)
{
	struct uvc_probe_ctrl_10 probe;
	struct uvc_probe_ctrl_10 res;
	uint32_t sent;
	tUSBRequest req;

	memset((void *) &res, 0, sizeof(res));

	req.bmRequestType = USB_RTYPE_DIR_OUT | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_PROBE_CONTROL_SET_CUR;
	req.wValue = (UVC_VS_PROBE_CONTROL << 8);
	req.wIndex = cam_inst.streaming_iface;
	req.wLength = sizeof(probe);

	probe.bmHint = 1;
	probe.bFormatIndex = 1;
	probe.bFrameIndex = 1;
	probe.dwFrameInterval = cam_inst.frame_interval;
	probe.wKeyFrameRate = 0;
	probe.wPFrameRate = 0;
	probe.wCompQuality = 0;
	probe.wCompWindowSize = 0;
	probe.wDelay = 0;
	probe.dwMaxVideoFrameSize = (1920 * 1080 * 3 * 8);
	probe.dwMaxPayloadTransferSize = 0;
	//probe.dwClockFrequency = 0;
	//probe.bmFramingInfo = 0;
	//probe.bPreferredVersion = 0;
	//probe.bMinVersion = 0;
	//probe.bMaxVersion = 0;

	// VS_PROBE_CONTROL(SET_CUR)
	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &probe,
			sizeof(probe),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);
	if (sent != sizeof(probe))
	{
		uvc_parsing_fault(0);
	}

	// VS_PROBE_CONTROL(GET_CUR)
	req.bmRequestType = USB_RTYPE_DIR_IN | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_PROBE_CONTROL_GET_CUR;
	req.wValue = (UVC_VS_PROBE_CONTROL << 8);
	req.wIndex = cam_inst.streaming_iface;
	req.wLength = sizeof(res);
	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &res,
			sizeof(res),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	// VS_COMMIT_CONTROL(SET_CUR)
	req.bmRequestType = USB_RTYPE_DIR_OUT | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_COMMIT_CONTROL_SET_CUR;
	req.wValue = (UVC_VS_COMMIT_CONTROL << 8);
	req.wIndex = cam_inst.streaming_iface;
	req.wLength = sizeof(probe);

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &res,
			sent, cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	return 0;
}

uint32_t uvc_probe_set_cur_15(void)
{
	struct uvc_probe_ctrl_15 probe;
	struct uvc_probe_ctrl_15 res;
	uint32_t sent;
	tUSBRequest req;

	req.bmRequestType = USB_RTYPE_DIR_OUT | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_PROBE_CONTROL_SET_CUR;
	req.wValue = (UVC_VS_PROBE_CONTROL << 8);
	req.wIndex = 1;
	req.wLength = sizeof(probe);

	probe.bmHint = 1;
	probe.bFormatIndex = 1;
	probe.bFrameIndex = 1;
	probe.dwFrameInterval = cam_inst.frame_interval;
	probe.wKeyFrameRate = 0;
	probe.wPFrameRate = 0;
	probe.wCompQuality = 1;
	probe.wCompWindowSize = 0;
	probe.wDelay = 0;
	probe.dwMaxVideoFrameSize = (1024 * 1024 * 8);
	probe.dwMaxPayloadTransferSize = cam_inst.device->sDeviceDescriptor.bMaxPacketSize0;
	probe.dwClockFrequency = 0;
	probe.bmFramingInfo = 0;
	probe.bPreferredVersion = 0;
	probe.bMinVersion = 0;
	probe.bMaxVersion = 0;
	probe.bUsage = 0;
	probe.bBitDepthLuma = 0;
	probe.bmSettings = 0;
	probe.bMaxNumberOfRefFramesPlus1 = 10; /* TODO make this not hard-coded */
	probe.bmRateControlModes = 0;
	probe.bmLayoutPerStream = 0;

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &probe,
			sizeof(probe),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);
	if (sent != sizeof(probe))
	{
		uvc_parsing_fault(0);
	}

	req.bmRequestType = USB_RTYPE_DIR_OUT | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_COMMIT_CONTROL_SET_CUR;
	req.wValue = (UVC_VS_COMMIT_CONTROL << 8);
	req.wIndex = 1;
	req.wLength = sizeof(probe);

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &probe,
			sizeof(probe),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);
	if (sent != sizeof(probe))
	{
		uvc_parsing_fault(1);
	}

	return 0;
}

uint32_t uvc_set_iface(void)
{
	USBHCDSetInterface(0, (uint32_t) cam_inst.device, cam_inst.streaming_iface,
		4);

	return 0;
}

size_t uvc_parse_uncomp_format_desc(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t subtype;
	uint8_t idx;
	uint8_t nframes;
	uint16_t *width;
	uint16_t *height;
	uint32_t *min_rate;
	uint32_t *max_rate;
	uint8_t ivals;
	uint32_t *ival;
	size_t i = 0;

	if (len != 27)
	{
		uvc_parsing_fault(0);
	}

	idx = buf[3];
	nframes = buf[4];

	cam_inst.uncomp_format_idx = idx;

	i += 27;
	while (i < max_len)
	{
		len = buf[i];
		subtype = buf[i + 2];
		idx = buf[i + 3];
		width = (uint16_t *) (buf + i + 5);
		height = (uint16_t *) (buf + i + 7);
		min_rate = (uint32_t *) (buf + i + 9);
		max_rate = (uint32_t *) (buf + i + 13);
		ivals = buf[i + 25];
		ival = (uint32_t *) (buf + i + 26);

		cam_inst.frame_interval = *ival;

		i += len;

		if (cam_inst.uncomp_frame_idx == 0)
		{
			cam_inst.uncomp_frame_idx = idx;
			break;
		}
	}

	return i;
}

void uvc_init(void (*uvc_frame_start_cb)(void),
	void (*uvc_frame_data_cb)(uint8_t *data, size_t len),
	void (*uvc_frame_end_cb)(void))
{

}
