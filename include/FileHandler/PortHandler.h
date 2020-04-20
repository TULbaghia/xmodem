#ifndef XMODEM_PORTHANDLER_H
#define XMODEM_PORTHANDLER_H

#include "defines.h"

#include <string>

using std::string;

class PortHandler {
private:
    HANDLE        portHandle;         // identyfikator portu
    LPCTSTR       portName;           // nazwa portu- Long Pointer to Const Tchar STRing
    DCB           portSettings{0};    // ustawienia portu szeregowego
    COMSTAT       portResources{};    // informacja o zasobach portu
    DWORD         portError{};        // reprezentuje wyjatek
    COMMTIMEOUTS  portTimings{0};     // parametry opoznienia
    USHORT        porttmpCRC{};       // suma kontrolna wyliczana przy transmisji
    BYTE          transmissionType;   // ASK | CRC
    BYTE          numberOfBytesToRead;
    unsigned long transmissionCharLength = sizeof(BYTE);
public:
    explicit PortHandler(const string& portName, const string& transmissionMode);

    void send(const string& fileName);

    void receive(const string& fileName);
};


#endif //XMODEM_PORTHANDLER_H
