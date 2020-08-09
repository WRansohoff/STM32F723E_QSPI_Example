#ifndef VVC_GLOBAL_H
#define VVC_GLOBAL_H

// Standard library includes.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Device header file.
#include "stm32f7xx.h"

// Global program variables.
uint32_t SystemCoreClock;
volatile uint32_t systick;
// Memory section boundaries which are defined in the linker script.
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;

// System call to enable standard library print functions.
int _write( int handle, char* data, int size );

// Helper method to perform blocking millisecond delays.
void delay_ms( uint32_t ms );

#endif
