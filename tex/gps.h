volatile int GPSSize;
volatile char GPS[100];

/**
 * @brief Function to be called when the GPS sends UART
 */
void UARTIntHandler(void);

/**
 * @brief Initializes the pins and the UART module on the TIVA for GPS
 */
void initGPS();
