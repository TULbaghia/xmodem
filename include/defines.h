#ifndef XMODEM_DEFINES_H
#define XMODEM_DEFINES_H

#include <windows.h>
#include <cstdint>

const BYTE SOH = 0x01;     // Start of Header
const BYTE EOT = 0x04;     // End of Transmission
const BYTE ACK = 0x06;     // Acknowledge
const BYTE NAK = 0x15;     // Not Acknowledge  -> Start Algebraic transfer
const BYTE ETB = 0x17;     // End of Transmission Block (Return to Amulet OS mode)
const BYTE CAN = 0x18;     // Cancel (Force receiver to start sending C's)
const BYTE C   = 0x43;     // ASCII 'C'        -> Start CRC transfer

#endif //XMODEM_DEFINES_H
