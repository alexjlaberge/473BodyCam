#include "uvc.h"

#include <stdlib.h>

#include <usblib/usblib.h>
#include <usblib/host/usbhost.h>
#include <usblib/host/usbhostpriv.h>

#define INT_BUFFER_SIZE 64
#define VIDEO_BUFFER_SIZE 256

#define UVC_VERSION 0x150
#define UVC_SC_UNDEFINED 0x00
#define UVC_SC_VIDEOCONTROL 0x01
#define UVC_SC_VIDEOSTREAMING 0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION 0x03

/**
 * @brief video class subtypes
 */
#define UVC_VC_DESCRIPTOR_UNDEFINED 0x00
#define UVC_VC_HEADER 0x01
#define UVC_VC_INPUT_TERMINAL 0x02
#define UVC_VC_OUTPUT_TERMINAL 0x03
#define UVC_VC_SELECTOR_UNIT 0x04
#define UVC_VC_PROCESSING_UNIT 0x05
#define UVC_VC_EXTENSION_UNIT 0x06
#define UVC_VC_ENCODING_UNIT 0x07
#define UVC_VS_UNDEFINED 0x00
#define UVC_VS_INPUT_HEADER 0x01
#define UVC_VS_OUTPUT_HEADER 0x02
#define UVC_VS_STILL_IMAGE_FRAME 0x03
#define UVC_VS_FORMAT_UNCOMPRESSED 0x04
#define UVC_VS_FRAME_UNCOMPRESSED 0x05
#define UVC_VS_FORMAT_MJPEG 0x06
#define UVC_VS_FRAME_MJPEG 0x07
#define UVC_VS_FORMAT_MPEG2TS 0x0A
#define UVC_VS_FORMAT_DV 0x0C
#define UVC_VS_COLORFORMAT 0x0D
#define UVC_VS_FORMAT_FRAME_BASED 0x10
#define UVC_VS_FRAME_FRAME_BASED 0x11
#define UVC_VS_FORMAT_STREAM_BASED 0x12
#define UVC_VS_FORMAT_H264 0x13
#define UVC_VS_FRAME_H264 0x14
#define UVC_VS_FORMAT_H264_SIMULCAST 0x15
#define UVC_VS_FORMAT_VP8 0x16
#define UVC_VS_FRAME_VP8 0x17
#define UVC_VS_FORMAT_VP8_SIMULCAST 0x18

/**
 * @brief Terminal types
 */
#define UVC_ITT_CAMERA 0x0201

/**
 * @brief some packet lengths
 */
#define UVC_CAM_TERM_SIZE 18
#define UVC_PROC_UNIT_SIZE 13
#define UVC_ENC_UNIT_SIZE 13
#define UVC_ISO_ENDPT_SIZE 7

#define USB_DTYPE_CS_ENDPOINT 0x25

#define UVC_VS_PROBE_CONTROL 0x01
#define UVC_VS_COMMIT_CONTROL 0x02
#define UVC_VS_PROBE_CONTROL_SET_CUR 0x01
#define UVC_VS_PROBE_CONTROL_GET_CUR 0x81
#define UVC_VS_COMMIT_CONTROL_SET_CUR 0x01

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

struct uvc_probe_ctrl_10
{
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
} PACKED;

struct uvc_probe_ctrl_15
{
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
	uint32_t dwClockFrequency;
	uint8_t bmFramingInfo;
	uint8_t bPreferredVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
	uint8_t bUsage;
	uint8_t bBitDepthLuma;
	uint8_t bmSettings;
	uint8_t bMaxNumberOfRefFramesPlus1;
	uint16_t bmRateControlModes;
	uint64_t bmLayoutPerStream;
} PACKED;

struct camera_device
{
	tUSBHostDevice *device;
	struct uvc_iad iad;
	uint32_t target_bit_rate;
	uint16_t uvc_version;

	void (*start_cb)(void);
	void (*data_cb)(uint8_t *buf, size_t len);
	void (*end_cb)(void);

	uint32_t stream_pipe;
	uint8_t stream_iface;
	uint8_t stream_iface_alt;
	uint8_t stream_endpt;
	uint8_t stream_format_idx;
	uint8_t stream_frame_idx;
	uint32_t stream_max_bandwidth;
	uint32_t stream_frame_interval;
	uint32_t stream_format;

	uint32_t interrupt_pipe;
	uint8_t interrupt_endpt;

	uint32_t frame_interval;
};

static struct camera_device cam_inst = {0};
uint8_t cam_stream_buf[VIDEO_BUFFER_SIZE];
uint8_t int_stream_buf[INT_BUFFER_SIZE];

void uvc_iad_init(struct uvc_iad *iad);
struct uvc_iad uvc_get_iad(void);

/**
 * @brief Called by usblib during device enumeration
 */
static void * uvc_open(tUSBHostDevice *dev);

/**
 * @brief Called by usblib during device shutdown
 */
static void uvc_close(void *dev);

void uvc_parse_received_packet(uint8_t *buf, size_t len);

/**
 * @brief Callback for pipe status changes
 */
static void uvc_pipe_cb(uint32_t pipe, uint32_t event);

uint32_t uvc_probe_set_cur_10(void);
uint32_t uvc_probe_set_cur_15(void);

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
 * @brief Parse an Encoding Unit Descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_extension_unit(uint8_t *buf, size_t max_len);

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
void uvc_parse_all_config(void);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_cs_vs_input_header(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_ctrl_endpoint(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_vsid(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_streaming_endpoint(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_mjpeg_format_desc(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_mjpeg_frame_desc(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_h264_format_desc(uint8_t *buf, size_t max_len);

/**
 * @brief Parse a Class Specific Video Streaming Input Header
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_h264_frame_desc(uint8_t *buf, size_t max_len);

/**
 * @brief An endless while to preserve state for debugging
 */
void uvc_parsing_fault(uint32_t i);

/**
 * @brief Entrypoint for parsing the control packets
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_control(uint8_t *buf, size_t max_len);

/**
 * @brief Entrypoint for parsing a group of streaming packets
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_streaming(uint8_t *buf, size_t max_len);

/**
 * @brief Entrypoint for parsing uncompressed format descriptor
 * @param buf Raw USB data buffer
 * @param max_len Maximum length to parse
 * @return The amount of data parsed
 */
size_t uvc_parse_uncomp_format_desc(uint8_t *buf, size_t max_len);
size_t uvc_parse_uncomp_frame_desc(uint8_t *buf, size_t max_len,
	uint8_t format_index);

uint32_t uvc_set_iface(void);

uint32_t uvc_probe_set_cur(void);

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

	if (cam_inst.device == NULL)
	{
		//iface = USBDescGetInterface(dev->psConfigDescriptor, 0, 0);

		cam_inst.device = dev;
		//cam_inst.in_use = 1;

		//for (i = 0; i < iface->bNumEndpoints; i++)
		//{
		//	endpt = USBDescGetInterfaceEndpoint(iface, i, 256);

		//	if (endpt != 0 && endpt->bmAttributes & USB_EP_ATTR_INT)
		//	{
		//		cam_inst.intr.pipe = USBHCDPipeAlloc(0, USBHCD_PIPE_INTR_IN,
		//				dev, uvc_pipe_cb);
		//		if (cam_inst.intr.pipe == 0)
		//		{
		//			goto fail;
		//		}

		//		err = USBHCDPipeConfig(cam_inst.intr.pipe, endpt->wMaxPacketSize,
		//				endpt->bInterval,
		//				endpt->bEndpointAddress & USB_EP_DESC_NUM_M);
		//		if (err != 0)
		//		{
		//			goto fail;
		//		}
		//	}
		//}

		uvc_parse_all_config();
		return &cam_inst;
	}

	return (void *) NULL;
}

static void uvc_close(void *dev)
{
	if (cam_inst.device != NULL)
	{
		USBHCDPipeFree(cam_inst.interrupt_pipe);
		USBHCDPipeFree(cam_inst.stream_pipe);
	}

	cam_inst.device = NULL;
	cam_inst.stream_pipe = 0;
	return;
}

static void uvc_pipe_cb(uint32_t pipe, uint32_t event)
{
	uint32_t len;
	uint8_t pkt_len;
	uint8_t pkt_type;
	uint8_t pkt_subtype;

	if (pipe == cam_inst.interrupt_pipe)
	{
		switch (event)
		{
		case USB_EVENT_ERROR:
			/* TODO error condition */
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
	else if (pipe == cam_inst.stream_pipe)
	{
		switch (event)
		{
		case USB_EVENT_SCHEDULER:
			len = USBHCDPipeSchedule(pipe, cam_stream_buf, VIDEO_BUFFER_SIZE);
			break;
		case USB_EVENT_RX_AVAILABLE:
			len = USBHCDPipeTransferSizeGet(pipe);
			uvc_parse_received_packet(cam_stream_buf, len);
			USBHCDPipeDataAck(pipe);
			break;
		default:
			uvc_parsing_fault(event);
			break;
		}
	}
	//else
	//{
	//	uvc_parsing_fault(15);
	//}
}

void uvc_parse_received_packet(uint8_t *buf, size_t len)
{
	static uint8_t fid = 2;

	if (cam_inst.stream_format == UVC_VS_FORMAT_UNCOMPRESSED)
	{
		if (fid != buf[1] & 0x01)
		{
			cam_inst.start_cb();
			fid = buf[1] & 0x01;
		}

		cam_inst.data_cb(buf + buf[0], len - buf[0]);

		if (buf[1] & 0x02)
		{
			cam_inst.end_cb();
		}
	}
}

void uvc_main(void)
{
	USBHCDPipeSchedule(cam_inst.stream_pipe, cam_stream_buf,
		VIDEO_BUFFER_SIZE);
}

void uvc_parse_all_config(void)
{
	size_t i;
	size_t procd;
	uint8_t *buf = (uint8_t *) cam_inst.device->psConfigDescriptor;
	size_t len = cam_inst.device->ui32ConfigDescriptorSize;

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

struct uvc_iad uvc_get_iad(void)
{
	struct uvc_iad ret = {0};
	tUSBRequest req;
	uint32_t sent;

	if (cam_inst.device == NULL)
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

	//if (max_len < UVC_OUT_TERM_MIN_LENGTH)
	//{
	//	return 0;
	//}

	//term.bLength = buf[0];
	//term.bDescriptorType = buf[1];
	//term.bDescriptorSubtype = buf[2];
	//term.bTerminalID = buf[3];

	//tmp = (uint16_t *) (buf + 4);

	//term.wTerminalType = *tmp;
	//term.bAssocTerminal = buf[6];
	//term.bSourceID = buf[7];
	//term.iTerminal = buf[8];

	//if (term.bLength < UVC_OUT_TERM_MIN_LENGTH ||
	//	term.bDescriptorType != USB_DTYPE_CS_INTERFACE ||
	//	term.bDescriptorSubtype != UVC_VC_OUTPUT_TERMINAL)
	//{
	//	return 0;
	//}

	//cam_inst.output_terminal = term;
	return buf[0];
}

size_t uvc_parse_camera_terminal(uint8_t *buf, size_t max_len)
{
	//uint16_t *tmp;
	//uint32_t *tmp32;

	//if (max_len < UVC_CAM_TERM_SIZE)
	//{
	//	return 0;
	//}

	//term.bLength = buf[0];
	//term.bDescriptorType = buf[1];
	//term.bDescriptorSubtype = buf[2];
	//term.bTerminalID = buf[3];

	//tmp = (uint16_t *) (buf + 4);
	//term.wTerminalType = *tmp;

	//term.bAssocTerminal = buf[6];
	//term.iTerminal = buf[7];

	//tmp = (uint16_t *) (buf + 8);
	//term.wObjectiveFocalLengthMin = *tmp;

	//tmp = (uint16_t *) (buf + 10);
	//term.wObjectiveFocalLengthMax = *tmp;

	//tmp = (uint16_t *) (buf + 12);
	//term.wOcularFocalLength = *tmp;

	//term.bControlSize = buf[14];

	//tmp32 = (uint32_t *) (buf + 15);
	//term.bmControls = *tmp & 0x00FFFFFF;

	//if (term.bLength != UVC_CAM_TERM_SIZE ||
	//	term.bDescriptorType != USB_DTYPE_CS_INTERFACE ||
	//	term.bDescriptorSubtype != UVC_VC_INPUT_TERMINAL)
	//{
	//	return 0;
	//}

	//cam_inst.camera_terminal = term;
	return buf[0];
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

	//if (max_len < UVC_PROC_UNIT_SIZE)
	//{
	//	return 0;
	//}

	len = buf[0];
	//if (buf[1] != USB_DTYPE_CS_INTERFACE ||
	//	buf[2] != UVC_VC_PROCESSING_UNIT)
	//{
	//	return 0;
	//}

	//cam_inst.proc_unit.bLength = buf[0];
	//cam_inst.proc_unit.bDescriptorType = buf[1];
	//cam_inst.proc_unit.bDescriptorSubtype = buf[2];
	//cam_inst.proc_unit.bUnitID = buf[3];
	//cam_inst.proc_unit.bSourceID = buf[4];

	//tmp = (uint16_t *) (buf + 5);
	//cam_inst.proc_unit.wMaxMultiplier = *tmp;

	//cam_inst.proc_unit.bControlSize = buf[7];

	//ctrl = (uint32_t *) (buf + 8);
	//cam_inst.proc_unit.bmControls = *ctrl & 0x00FFFFFF;
	//cam_inst.proc_unit.iProcessing = buf[11];
	//cam_inst.proc_unit.bmVideoStandards = buf[12];

	return len;
}

size_t uvc_parse_encoding_unit(uint8_t *buf, size_t max_len)
{
	uint32_t *tmp;

	//if (max_len < UVC_ENC_UNIT_SIZE)
	//{
	//	return 0;
	//}

	//if (buf[0] != UVC_ENC_UNIT_SIZE ||
	//	buf[1] != USB_DTYPE_CS_INTERFACE ||
	//	buf[2] != UVC_VC_ENCODING_UNIT)
	//{
	//	return 0;
	//}

	//cam_inst.enc_unit.bLength = buf[0];
	//cam_inst.enc_unit.bDescriptorType = buf[1];
	//cam_inst.enc_unit.bDescriptorSubtype = buf[2];

	//cam_inst.enc_unit.bUnitID = buf[3];
	//cam_inst.enc_unit.bSourceID = buf[4];
	//cam_inst.enc_unit.iEncoding = buf[5];
	//cam_inst.enc_unit.bControlSize = buf[6];

	//tmp = (uint32_t *) (buf + 7);
	//cam_inst.enc_unit.bmControls = *tmp & 0x00FFFFFF;

	//tmp = (uint32_t *) (buf + 10);
	//cam_inst.enc_unit.bmControlsRuntime = *tmp & 0x00FFFFFF;

	return buf[0];
}

size_t uvc_parse_isoc_endpoint(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint16_t *tmp;

	if (max_len < len)
	{
		return 0;
	}

	len = buf[0];
	//endpt.bDescriptorType = buf[1];
	//endpt.bEndpointAddress = buf[2];
	//endpt.bmAttributes = buf[3];

	//tmp = (uint16_t *) (buf + 4);
	//endpt.wMaxPacketSize = *tmp;

	//endpt.bInterval = buf[6];

	//if (endpt.bDescriptorType == USB_DTYPE_ENDPOINT &&
	//	(endpt.bEndpointAddress & USB_RTYPE_DIR_IN) &&
	//	(endpt.bmAttributes & 0x03))
	//{
		return buf[0];
	//}

	//return 0;
}

size_t uvc_parse_extension_unit(uint8_t *buf, size_t max_len)
{
	return buf[0];
}

void uvc_parsing_fault(uint32_t i)
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

	total_length = (uint16_t *) (buf + 4);
	cam_inst.stream_endpt = buf[6];

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

	if (cam_inst.device != NULL)
	{
		//cam_inst.stream_iface_alt = alternate_setting;
		//cam_inst.stream_iface = buf[2];
		cam_inst.stream_iface_alt = 1;
		cam_inst.stream_iface = 1;
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

	if ((addr & 0x80) == 0)
	{
		return len;
	}

	if ((attr & 0x03) != 0x01)
	{
		return len;
	}

	if (cam_inst.stream_pipe != 0)
	{
		return len;
	}

	cam_inst.stream_endpt = addr & 0x0F;

	cam_inst.stream_pipe = USBHCDPipeAllocSize(0, USBHCD_PIPE_ISOC_IN_DMA,
			cam_inst.device, *max_packet_size, uvc_pipe_cb);

	err = USBHCDPipeConfig(cam_inst.stream_pipe, *max_packet_size,
			interval,
			cam_inst.stream_endpt);

	return len;
}

uint32_t uvc_probe_set_cur(void)
{
	switch (cam_inst.uvc_version)
	{
	case 0x100:
		return uvc_probe_set_cur_10();
	case 0x150:
		return uvc_probe_set_cur_15();
	default:
		uvc_parsing_fault(cam_inst.uvc_version);
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
	req.wIndex = cam_inst.stream_iface;
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
	req.wIndex = cam_inst.stream_iface;
	req.wLength = sizeof(res);
	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &res,
			sizeof(res),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	// VS_COMMIT_CONTROL(SET_CUR)
	req.bmRequestType = USB_RTYPE_DIR_OUT | USB_RTYPE_CLASS |
		USB_RTYPE_INTERFACE;
	req.bRequest = UVC_VS_COMMIT_CONTROL_SET_CUR;
	req.wValue = (UVC_VS_COMMIT_CONTROL << 8);
	req.wIndex = cam_inst.stream_iface;
	req.wLength = sizeof(probe);

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &res,
			sent, cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);

	return 0;
}

uint32_t uvc_probe_set_cur_15(void)
{
	struct uvc_probe_ctrl_15 probe;
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
	USBHCDSetInterface(0, (uint32_t) cam_inst.device, cam_inst.stream_iface,
		cam_inst.stream_iface_alt);

	return 0;
}

size_t uvc_parse_uncomp_format_desc(uint8_t *buf, size_t max_len)
{
	uint8_t len = buf[0];
	uint8_t type = buf[1];
	uint8_t subtype = buf[2];
	uint8_t idx;
	size_t i = 0;

	if (len != 27 ||
		type != USB_DTYPE_CS_INTERFACE ||
		subtype != UVC_VS_FORMAT_UNCOMPRESSED)
	{
		uvc_parsing_fault(0);
	}

	idx = buf[3];

	i += len;
	while (i < max_len)
	{
		size_t procd = uvc_parse_uncomp_frame_desc(buf + i, max_len - i, idx);
		if (procd != 0)
		{
			i += procd;
		}
		else
		{
			break;
		}
	}

	return i;
}

size_t uvc_parse_uncomp_frame_desc(uint8_t *buf, size_t max_len,
	uint8_t format_index)
{
	uint8_t len = buf[0];
	uint8_t type = buf[1];
	uint8_t subtype = buf[2];
	uint8_t idx;
	uint32_t *max_rate;
	uint32_t *ival;

	if (len < 26 ||
		type != USB_DTYPE_CS_INTERFACE ||
		subtype != UVC_VS_FRAME_UNCOMPRESSED)
	{
		return 0;
	}

	idx = buf[3];
	max_rate = (uint32_t *) (buf + 13);
	ival = (uint32_t *) (buf + 26);

	if (*max_rate > cam_inst.stream_max_bandwidth &&
		*max_rate <= cam_inst.target_bit_rate &&
		((cam_inst.stream_format == 0) ||
		(cam_inst.stream_format == UVC_VS_FORMAT_UNCOMPRESSED)))
	{
		cam_inst.stream_format_idx = format_index;
		cam_inst.stream_frame_idx = idx;
		cam_inst.stream_format = UVC_VS_FORMAT_UNCOMPRESSED;
		cam_inst.stream_max_bandwidth = *max_rate;
		cam_inst.frame_interval = *ival;
	}

	return len;
}

void uvc_init(uint32_t target_bit_rate,
	void (*uvc_frame_start_cb)(void),
	void (*uvc_frame_data_cb)(const uint8_t *buf, size_t len),
	void (*uvc_frame_end_cb)(void))
{
	cam_inst.start_cb = uvc_frame_start_cb;
	cam_inst.data_cb = uvc_frame_data_cb;
	cam_inst.end_cb = uvc_frame_end_cb;
	cam_inst.target_bit_rate = target_bit_rate;
}
