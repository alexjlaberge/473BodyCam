#ifndef UVC_H_
#define UVC_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <driverlib/usb.h>

#include <usblib/usblib.h>
#include <usblib/host/usbhost.h>
#include <usblib/host/usbhostpriv.h>

extern const tUSBHostClassDriver uvc_driver;

/**
 * @brief Call this to initialize the UVC driver
 * @param target_bandwidth Desired bit rate
 * @param uvc_frame_start_cb Called when a new frame is started
 * @param uvc_frame_data_cb Called when frame data has arrived
 * @param uvc_frame_end_cb Called when a frame has ended
 *
 * @details This function performs all of the required initialization for the
 * UVC driver.
 */
void uvc_init(uint32_t target_bit_rate,
	void (*uvc_frame_start_cb)(void),
	void (*uvc_frame_data_cb)(const uint8_t *buf, size_t len),
	void (*uvc_frame_end_cb)(void));

/**
 * @brief This function must be called periodically to process UVC data
 */
void uvc_main(void);

int uvc_is_mjpeg(void);

int uvc_is_uncomp(void);

#endif /* UVC_H_ */
