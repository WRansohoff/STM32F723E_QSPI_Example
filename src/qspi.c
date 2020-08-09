#include "qspi.h"

// Use 'status-polling' mode to wait for Flash register status.
void qspi_reg_wait( uint8_t reg, uint32_t msk, uint32_t mat ) {
  // Disable the peripheral.
  QUADSPI->CR   &= ~( QUADSPI_CR_EN );
  // Set the 'mask', 'match', and 'polling interval' values.
  QUADSPI->PSMKR = msk;
  QUADSPI->PSMAR = mat;
  QUADSPI->PIR   = 0x10;
  // Set the 'auto-stop' bit to end the transaction after a match.
  QUADSPI->CR   |=  ( QUADSPI_CR_APMS );
  // Clear instruction, mode and transaction phases.
  QUADSPI->CCR  &= ~( QUADSPI_CCR_INSTRUCTION |
                      QUADSPI_CCR_FMODE |
                      QUADSPI_CCR_IMODE |
                      QUADSPI_CCR_DMODE |
                      QUADSPI_CCR_ADMODE );
  // Set 4-wire instruction and data modes, and auto-polling mode.
  QUADSPI->CCR  |=  ( ( 3 << QUADSPI_CCR_IMODE_Pos ) |
                      ( 3 << QUADSPI_CCR_DMODE_Pos ) |
                      ( 2 << QUADSPI_CCR_FMODE_Pos ) );
  // Enable the peripheral.
  QUADSPI->CR   |=  ( QUADSPI_CR_EN );
  // Set the given 'read register' instruction to start polling.
  QUADSPI->CCR  |=  ( reg << QUADSPI_CCR_INSTRUCTION_Pos );
  // Wait for a match.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  // Acknowledge the 'status match flag.'
  QUADSPI->FCR |=  ( QUADSPI_FCR_CSMF );
  // Un-set the data mode and disable auto-polling.
  QUADSPI->CCR  &= ~( QUADSPI_CCR_FMODE |
                      QUADSPI_CCR_DMODE );
  // Disable the peripheral.
  QUADSPI->CR   &= ~( QUADSPI_CR_EN );
}

// Enable writes on the QSPI Flash. Must be done before every
// erase / program operation.
void qspi_wen() {
  // Disable the peripheral.
  QUADSPI->CR   &= ~( QUADSPI_CR_EN );
  // Clear the instruction, mode, and transaction phases.
  QUADSPI->CCR  &= ~( QUADSPI_CCR_INSTRUCTION |
                      QUADSPI_CCR_FMODE |
                      QUADSPI_CCR_IMODE |
                      QUADSPI_CCR_DMODE |
                      QUADSPI_CCR_ADMODE );
  // Set 4-wire instruction mode.
  QUADSPI->CCR  |=  ( 3 << QUADSPI_CCR_IMODE_Pos );
  // Enable the peripheral and send the 'write enable' command.
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );
  QUADSPI->CCR |=  ( 0x06 << QUADSPI_CCR_INSTRUCTION_Pos );
  // Wait for the transaction to finish.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  // Disable the peripheral.
  QUADSPI->CR   &= ~( QUADSPI_CR_EN );
  // Wait until 'writes enabled' is set in the config register.
  qspi_reg_wait( 0x05, 0x43, 0x42 );
}

// Erase a 4KB sector. Sector address = ( snum * 0x1000 )
void qspi_erase_sector( uint32_t snum ) {
  // Send 'enable writes' command.
  qspi_wen();
  // Erase the sector, and wait for the operation to complete.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CCR  &= ~( QUADSPI_CCR_INSTRUCTION |
                      QUADSPI_CCR_FMODE |
                      QUADSPI_CCR_IMODE |
                      QUADSPI_CCR_DMODE |
                      QUADSPI_CCR_ADMODE );
  QUADSPI->CCR |=  ( ( 3 << QUADSPI_CCR_IMODE_Pos ) |
                     ( 3 << QUADSPI_CCR_ADMODE_Pos ) );
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );
  // 0x20 is the "sector erase" command.
  QUADSPI->CCR |=  ( 0x20 << QUADSPI_CCR_INSTRUCTION_Pos );
  // The address is equal to the sector number * 4KB.
  QUADSPI->AR   =  ( snum * 0x1000 );
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  // Disable the peripheral once the transaction is complete.
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  // Wait for the 'write in progress' bit to clear.
  qspi_reg_wait( 0x05, 0x43, 0x40 );
}

// Write one word of data (4 bytes) to a QSPI Flash chip.
void qspi_write_word( uint32_t addr, uint32_t data ) {
  // Send 'enable writes' command.
  qspi_wen();
  // Write the word of data.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CCR  &= ~( QUADSPI_CCR_INSTRUCTION |
                      QUADSPI_CCR_FMODE |
                      QUADSPI_CCR_IMODE |
                      QUADSPI_CCR_DMODE |
                      QUADSPI_CCR_ADMODE );
  QUADSPI->CCR |=  ( ( 3 << QUADSPI_CCR_IMODE_Pos ) |
                     ( 3 << QUADSPI_CCR_ADMODE_Pos ) |
                     ( 3 << QUADSPI_CCR_DMODE_Pos ) );
  // Set data length (3 + 1 = 4 bytes).
  QUADSPI->DLR = 3;
  // Enable the peripheral and set instruction, address, data.
  QUADSPI->CR  |=  ( QUADSPI_CR_EN );
  QUADSPI->CCR |=  ( 0x12 << QUADSPI_CCR_INSTRUCTION_Pos );
  QUADSPI->AR   =  ( addr );
  QUADSPI->DR   =  ( data );
  // Wait for the transaction to complete, and disable the peripheral.
  while ( QUADSPI->SR & QUADSPI_SR_BUSY ) {};
  QUADSPI->CR  &= ~( QUADSPI_CR_EN );
  // Clear the data length register.
  QUADSPI->DLR = 0;
  // Wait for the 'write in progress' bit to clear.
  qspi_reg_wait( 0x05, 0x41, 0x40 );
}
