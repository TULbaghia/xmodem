#ifndef XMODEM_PORTHANDLER_H
#define XMODEM_PORTHANDLER_H

#include "defines.h"
#include <string>

using std::string;

class PortHandler {
private:
    HANDLE        portHandle;          // identyfikator portu
    DCB           portSettings{0};     // ustawienia portu szeregowego
    COMSTAT       portResources{};     // informacja o zasobach portu
    DWORD         portError{};         // reprezentuje wyjatek
    COMMTIMEOUTS  portTimings{0};      // parametry opoznienia

    BYTE          transmissionType;    // typ transmisji danych  NAK | CRC
    BYTE          numberOfBytesToRead; // rozmiar ramki         132B | 133B

    unsigned long bitsLengthInChar = sizeof(BYTE); // ilosc bitow przypadajaca na jeden bajt
public:
    explicit PortHandler(const string& portName, const string& transmissionMode);

    void send(const string& fileName);

    void receive(const string& fileName);
};


#endif //XMODEM_PORTHANDLER_H
