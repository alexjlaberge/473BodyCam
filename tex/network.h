#define WIFI_SSID "dd-wrt"
#define WIFI_PASS "roar2015"

#define WIFI_PKT_TYPE_MJPEG 2
#define WIFI_PKT_TYPE_UNCOMP 4	

#define WIFI_HEADER_SIZE 8
#define WIFI_PACKET_SIZE (256 - WIFI_HEADER_SIZE)	

/**
 * @brief This function gets called to send a packet
 * @param buf Holds the data that is to be packetized and sent
 * @param size Holds the size of the data being sent
 */
int WIFI_sendUSBData(const uint8_t * buf, size_t size);

/**
 * @brief This function gets called to send a GPS packet
 */
int WIFI_sendGPSData();
