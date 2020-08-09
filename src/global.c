#include "global.h"

uint32_t SystemCoreClock = 16000000;
volatile uint32_t systick = 0;

// System call to support standard library print functions.
int _write( int handle, char* data, int size ) {
  int count = size;
  while( count-- ) {
    while( !( USART6->ISR & USART_ISR_TXE ) ) {};
    USART6->TDR = *data++;
  }
  return size;
}

// Delay for a specified number of milliseconds.
// TODO: Prevent rollover bug on the 'systick' value.
void delay_ms( uint32_t ms ) {
  // Calculate the 'end of delay' tick value, then wait for it.
  uint32_t next = systick + ms;
  while ( systick < next ) { __WFI(); }
}
