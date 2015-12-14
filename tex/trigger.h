/**
 * @brief Configures the TIVA pins to trigger on our TIVA_WAKE signal
 */
void initTrigger();

/**
 * @brief Function to be called when TIVA_WAKE changes
 */
void KIntHandler(void);

