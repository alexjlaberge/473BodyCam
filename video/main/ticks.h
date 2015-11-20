/*
 * ticks.h
 *
 *  Created on: Oct 25, 2015
 *      Author: Alec
 */

#ifndef TICKS_H_
#define TICKS_H_

#include <stdint.h>

void sys_tick_handler(void);

uint32_t ticks_since_last_call(void);

#endif /* TICKS_H_ */
