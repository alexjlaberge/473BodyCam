/*
 * uvc.c
 *
 *  Created on: Oct 21, 2015
 *      Author: Alec
 */

#include "uvc.h"
#include <usblib/usblib.h>
#include <usblib/host/usbhost.h>

#define UVC_CTRL_ENDPOINT 0

static void * uvc_open(tUSBHostDevice *dev);
static void uvc_close(void *dev);

const tUSBHostClassDriver uvc_driver =
{
    USB_CLASS_VIDEO,
    uvc_open,
    uvc_close,
    0
};

void * uvc_open(tUSBHostDevice *dev)
{
	return (void *) 0;
}

void uvc_close(void *dev)
{
	return;
}