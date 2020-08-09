#include "main.h"

// Reset handler: set the stack pointer and branch to main().
__attribute__( ( naked ) ) void reset_handler( void ) {
  // Set the stack pointer to the 'end of stack' value.
  __asm__( "LDR r0, =_estack\n\t"
           "MOV sp, r0" );
  // Branch to main().
  __asm__( "B main" );
}

/**
 * Main program.
 */
int main( void ) {
  // Copy initialized data from .sidata (Flash) to .data (RAM)
  memcpy( &_sdata, &_sidata, ( ( void* )&_edata - ( void* )&_sdata ) );
  // Clear the .bss section in RAM.
  memset( &_sbss, 0x00, ( ( void* )&_ebss - ( void* )&_sbss ) );

  // Enable floating-point unit.
  SCB->CPACR    |=  ( 0xF << 20 );

  // Set clock speed to 216MHz (each tick is a bit less than 5ns)
  // PLL out = ( 16MHz * ( N / M ) / P ). P = 2, N = 54, M = 2.
  FLASH->ACR   |=  ( 7 << FLASH_ACR_LATENCY_Pos );
  RCC->PLLCFGR &= ~( RCC_PLLCFGR_PLLN |
                     RCC_PLLCFGR_PLLM );
  RCC->PLLCFGR |=  ( ( 54 << RCC_PLLCFGR_PLLN_Pos ) |
                     ( 2 << RCC_PLLCFGR_PLLM_Pos ) );
  RCC->CR      |=  ( RCC_CR_PLLON );
  while ( !( RCC->CR & RCC_CR_PLLRDY ) ) {};
  RCC->CFGR    |=  ( 2 << RCC_CFGR_SW_Pos );
  while ( ( RCC->CFGR & RCC_CFGR_SWS ) != ( 2 << RCC_CFGR_SWS_Pos ) ) {};
  SystemCoreClock = 216000000;

  // Enable peripheral clocks: GPIOB-E, QSPI, USART6.
  RCC->AHB1ENR |=  ( RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN |
                     RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN );
  RCC->AHB3ENR |=  ( RCC_AHB3ENR_QSPIEN );
  RCC->APB2ENR |=  ( RCC_APB2ENR_USART6EN );

  // Initialize pins C6 and C7 for USART6.
  GPIOC->MODER    |=  ( ( 2 << ( 6 * 2 ) ) |
                        ( 2 << ( 7 * 2 ) ) );
  GPIOC->OSPEEDR  |=  ( ( 2 << ( 6 * 2 ) ) |
                        ( 2 << ( 7 * 2 ) ) );
  GPIOC->AFR[ 0 ] |=  ( ( 8 << ( 6 * 4 ) ) |
                        ( 8 << ( 7 * 4 ) ) );
  // Initialize pins B2, B6, C9, C10, D13, E2 for QSPI.
  GPIOB->MODER    |=  ( ( 2 << ( 2 * 2 ) ) |
                        ( 2 << ( 6 * 2 ) ) );
  GPIOB->OSPEEDR  |=  ( ( 3 << ( 2 * 2 ) ) |
                        ( 3 << ( 6 * 2 ) ) );
  GPIOB->PUPDR    |=  ( 1 << ( 6 * 2 ) );
  GPIOB->AFR[ 0 ] |=  ( ( 9 << ( 2 * 4 ) ) |
                        ( 10 << ( 6 * 4 ) ) );
  GPIOC->MODER    |=  ( ( 2 << ( 9 * 2 ) ) |
                        ( 2 << ( 10 * 2 ) ) );
  GPIOC->OSPEEDR  |=  ( ( 3 << ( 9 * 2 ) ) |
                        ( 3 << ( 10 * 2 ) ) );
  GPIOC->AFR[ 1 ] |=  ( ( 9 << ( ( 9 - 8 ) * 4 ) ) |
                        ( 9 << ( ( 10 - 8 ) * 4 ) ) );
  GPIOD->MODER    |=  ( 2 << ( 13 * 2 ) );
  GPIOD->OSPEEDR  |=  ( 3 << ( 13 * 2 ) );
  GPIOD->AFR[ 1 ] |=  ( 9 << ( ( 13 - 8 ) * 4 ) );
  GPIOE->MODER    |=  ( 2 << ( 2 * 2 ) );
  GPIOE->OSPEEDR  |=  ( 3 << ( 2 * 2 ) );
  GPIOE->AFR[ 0 ] |=  ( 9 << ( 2 * 4 ) );

  // Setup USART6 for 115200-baud TX.
  USART6->BRR  =  ( SystemCoreClock / 115200 );
  USART6->CR1 |=  ( USART_CR1_UE | USART_CR1_TE );

  // QSPI peripheral initialization.
  // Set Flash size; 512Mb = 64MB = 2^(25+1) bytes.
  QUADSPI->DCR |=  ( 25 << QUADSPI_DCR_FSIZE_Pos );
  // Set 1-wire data mode with 32-bit addressing.
  QUADSPI->CCR |=  ( ( 3 << QUADSPI_CCR_ADSIZE_Pos ) |
                     ( 1 << QUADSPI_CCR_IMODE_Pos ) );
  // Wait an extra half-cycle to read, and set a clock prescaler.
  QUADSPI->CR  |=  ( QUADSPI_CR_SSHIFT |
                     ( 2 << QUADSPI_CR_PRESCALER_Pos ) );

  // Flash chip initialization.
  // Send 'enter QSPI mode' command.
  // Enable the peripheral.
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );
  // Set the 'enter QSPI mode' instruction.
  QUADSPI->CCR |=  ( 0x35 << QUADSPI_CCR_INSTRUCTION_Pos );
  // Wait for the transaction to complete, and disable the peripheral.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  // Wait for the 'QSPI mode enabled' bit.
  qspi_reg_wait( 0x05, 0x41, 0x40 );

  // Send 'enable 4-byte addressing' command.
  // The peripheral may start a new transfer as soon as the
  // 'instruction' field is written, so it is safest to disable
  // the peripheral before clearing that field.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  QUADSPI->CCR &= ~( QUADSPI_CCR_INSTRUCTION );
  // Use all 4 data lines to send the instruction.
  QUADSPI->CCR |=  ( 3 << QUADSPI_CCR_IMODE_Pos );
  // Enable the peripheral and send the 'enable 4B addresses' command.
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );
  QUADSPI->CCR |=  ( 0xB7 << QUADSPI_CCR_INSTRUCTION_Pos );
  // Wait for the transaction to complete, and disable the peripheral.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  // Wait for the '4-byte addressing enabled' bit to be set.
  qspi_reg_wait( 0x15, 0x20, 0x20 );

  // Test writing some data.
  // No need to run this every time; Flash is non-volatile, but it
  // has limited "write endurance" on the order of ~10k-100k cycles.
  qspi_erase_sector( 0 );
  qspi_write_word( 0, 0x01234567 );
  qspi_write_word( 4, 0x89ABCDEF );

  // Enable memory-mapped mode. MX25L512 Flash chips use
  // 6 "dummy cycles" with Quad I/O "fast read" instructions by
  // default, which allows up to 84MHz communication speed.
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  QUADSPI->CCR &= ~( QUADSPI_CCR_INSTRUCTION );
  QUADSPI->CCR |= ( 3 << QUADSPI_CCR_FMODE_Pos |
                    3 << QUADSPI_CCR_ADMODE_Pos |
                    3 << QUADSPI_CCR_DMODE_Pos |
                    3 << QUADSPI_CCR_IMODE_Pos |
                    0xEC << QUADSPI_CCR_INSTRUCTION_Pos |
                    6 << QUADSPI_CCR_DCYC_Pos );
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );

  // Add a dummy cycle; if memory-mapped access is attempted
  // immediately after enabling the peripheral, it seems to fail.
  // I'm not sure why, but adding one nop instruction seems to fix it.
  __asm( "NOP" );

  // Test reading values from memory-mapped Flash.
  int val = *( ( uint32_t* ) 0x90000000 );
  printf( "QSPI[0]: 0x%08X\r\n", val );
  val = *( ( uint32_t* ) 0x90000002 );
  printf( "QSPI[2]: 0x%08X\r\n", val );
  val = *( ( uint32_t* ) 0x90000008 );
  printf( "QSPI[8]: 0x%08X\r\n", val );

  // Done; empty main loop.
  while( 1 ) {};
  return 0; // lol
}
