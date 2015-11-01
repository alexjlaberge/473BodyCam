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

/* TODO Add more members as necessary */
struct camera_device
{
	tUSBHostDevice *device;
	uint8_t in_use;
	uint32_t interrupt_in_pipe;
};

static struct camera_device cam_inst = {0, 0};

static void * uvc_open(tUSBHostDevice *dev);
static void uvc_close(void *dev);
static uint32_t uvc_callback(void *camera, uint32_t event, uint32_t msg_param,
		void *msg_data);
static void uvc_int_in_callback(uint32_t pipe, uint32_t event);

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
	tEndpointDescriptor *endpoint_desc;
	int i;

	iface = USBDescGetInterface(dev->psConfigDescriptor, 0, 0);

	if (!cam_inst.in_use)
	{
		cam_inst.device = dev;
		cam_inst.in_use = 1;

		for (i = 0; i < iface->bNumEndpoints; i++)
		{
			endpoint_desc = USBDescGetInterfaceEndpoint(iface, i, 256);
			if (endpoint_desc == 0)
			{
				break;
			}

			/* TODO actually set up the proper endpoints */
			/* TODO we are using isochronous transfers, not interrupts/bulk */

			if ((endpoint_desc->bmAttributes & USB_EP_ATTR_TYPE_M) ==
					USB_EP_ATTR_INT)
			{
				if (endpoint_desc->bEndpointAddress & USB_EP_DESC_IN)
				{
					cam_inst.interrupt_in_pipe = USBHCDPipeAlloc(0,
							USBHCD_PIPE_INTR_IN, dev, uvc_int_in_callback);
					USBHCDPipeConfig(cam_inst.interrupt_in_pipe,
							endpoint_desc->wMaxPacketSize,
							endpoint_desc->bInterval,
							endpoint_desc->bEndpointAddress & USB_EP_DESC_NUM_M
							);
				}
			}
		}

		return &cam_inst;
	}

	return (void *) 0;
}

static void uvc_close(void *dev)
{
	return;
}

static void uvc_int_in_callback(uint32_t pipe, uint32_t event)
{
	return;
}

uint32_t uvc_callback(void *camera, uint32_t event, uint32_t msg_param,
		void *msg_data)
{
	return 0;
}
