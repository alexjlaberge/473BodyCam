/*
 * ticks.c
 *
 *  Created on: Oct 25, 2015
 *      Author: Alec
 */

#include "ticks.h"

volatile uint32_t ticks = 0;

void sys_tick_handler(void)
{
	ticks++;
}

uint32_t ticks_since_last_call(void)
{
	uint32_t ret = ticks;
	ticks = 0;
	return ret;
}
