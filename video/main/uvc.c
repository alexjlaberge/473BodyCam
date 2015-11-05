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

/* TODO Add more members as necessary */
struct camera_device
{
	tUSBHostDevice *device;
	uint8_t has_error;
	uint8_t in_use;
	struct usb_pipe intr;
	struct usb_pipe stream;
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

tConfigDescriptor uvc_get_config(void)
{
	tConfigDescriptor conf = {0};
	tUSBRequest req;
	uint32_t sent;

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

	return conf;
}

uvc_enc_term_desc_init(struct uvc_enc_term_desc *desc)
{
	size_t i = 0;

	desc->bControlSize = 0;
	desc->bDescriptorSubType = 0;
	desc->bDescriptorType = 0;
	desc->bUnitID = 0;
	desc->bSourceID = 0;
	desc->iEncoding = 0;
	desc->bControlSize = 0;

	for (i = 0; i < 3; i++)
	{
		desc->bmControls[i] = 0;
		desc->bmControlsRuntime[i] = 0;
	}
}

struct uvc_enc_term_desc uvc_get_enc_term_desc(void)
{
	struct uvc_enc_term_desc ret = {0};
	tUSBRequest req;
	uint32_t sent;

	uvc_enc_term_desc_init(&ret);

	/* TODO actually implement the request */

	if (!cam_inst.in_use)
	{
		return ret;
	}

	sent = USBHCDControlTransfer(0, &req, cam_inst.device, (uint8_t *) &ret,
			sizeof(struct uvc_enc_term_desc),
			cam_inst.device->sDeviceDescriptor.bMaxPacketSize0);
	if (sent != sizeof(struct uvc_enc_term_desc))
	{
		goto fail;
	}

	return ret;

fail:
	uvc_enc_term_desc_init(&ret);
	return ret;
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
