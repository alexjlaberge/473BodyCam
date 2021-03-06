/**
 * @brief Call this to initialize the UVC driver
 * @param target_bandwidth Desired bit rate
 * @param uvc_frame_start_cb Called when a new frame is started
 * @param uvc_frame_data_cb Called when frame data has arrived
 * @param uvc_frame_end_cb Called when a frame has ended
 */
void uvc_init(uint32_t target_bit_rate,
	void (*uvc_frame_start_cb)(void),
	void (*uvc_frame_data_cb)(const uint8_t *buf, size_t len),
	void (*uvc_frame_end_cb)(void));

/**
 * @brief This function must be called periodically
 */
void uvc_main(void);

/**
 * @brief Check if the UVC driver is yielding MJPEG video
 */
int uvc_is_mjpeg(void);

/**
 * @brief Check if the UVC driver is yielding YUY2 video
 */
int uvc_is_uncomp(void);
