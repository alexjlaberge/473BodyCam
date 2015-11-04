/*
 * uvc.c
 *
 *  Created on: Oct 21, 2015
 *      Author: Alec
 */

#include "uvc.h"
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
	struct usb_pipe ctrl;
	tUSBHostDevice *device;
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
	int i;

	iface = USBDescGetInterface(dev->psConfigDescriptor, 0, 0);

	if (!cam_inst.in_use)
	{
		cam_inst.device = dev;
		cam_inst.in_use = 1;

		for (i = 0; i < iface->bNumEndpoints; i++)
		{
			endpt = USBDescGetInterfaceEndpoint(iface, i, 256);
			if (endpt == 0)
			{
				break;
			}

			if ((endpt->bmAttributes & USB_EP_ATTR_ISOC) &&
					(endpt->bEndpointAddress & USB_EP_DESC_IN))
			{
				cam_inst.stream.pipe = USBHCDPipeAlloc(0, USBHCD_PIPE_ISOC_IN,
						dev, uvc_pipe_cb);
				if (cam_inst.stream.pipe == 0)
				{
					goto fail;
				}

				USBHCDPipeConfig(cam_inst.stream.pipe, endpt->wMaxPacketSize,
						endpt->bInterval,
						endpt->bEndpointAddress & USB_EP_DESC_NUM_M);
			}
			else if (endpt->bmAttributes & USB_EP_ATTR_CONTROL)
			{
				cam_inst.ctrl.pipe = USBHCDPipeAlloc(0, USBHCD_PIPE_CONTROL,
						dev, uvc_pipe_cb);
				if (cam_inst.ctrl.pipe == 0)
				{
					goto fail;
				}

				USBHCDPipeConfig(cam_inst.ctrl.pipe, endpt->wMaxPacketSize,
						endpt->bInterval,
						endpt->bEndpointAddress & USB_EP_DESC_NUM_M);
			}
			else if (endpt->bmAttributes & USB_EP_ATTR_INT)
			{
				cam_inst.intr.pipe = USBHCDPipeAlloc(0, USBHCD_PIPE_INTR_IN,
						dev, uvc_pipe_cb);
				if (cam_inst.intr.pipe == 0)
				{
					goto fail;
				}

				USBHCDPipeConfig(cam_inst.intr.pipe, endpt->wMaxPacketSize,
						endpt->bInterval,
						endpt->bEndpointAddress & USB_EP_DESC_NUM_M);
			}
		}

		if (cam_inst.ctrl.pipe == 0 || cam_inst.stream.pipe == 0)
		{
//			goto fail;
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
	return;
}

uint32_t uvc_callback(void *camera, uint32_t event, uint32_t msg_param,
		void *msg_data)
{
	return 0;
}
