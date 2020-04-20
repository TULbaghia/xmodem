#include "PortHandler.h"
#include "DataBlock.h"
#include <iostream>
#include <deque>

using std::cout;
using std::ofstream;
using std::ios;
using std::ifstream;
using std::deque;
using std::istreambuf_iterator;

PortHandler::PortHandler(const string& portName, const string& transmissionMode) : transmissionType(transmissionMode == "C" ? C : NAK), numberOfBytesToRead(transmissionMode == "C" ? 133 : 132) {
    this->portHandle = CreateFile(portName.c_str(),                // Nazwa zasobu ktory ma zostac uzyty
                                  GENERIC_READ | GENERIC_WRITE,    // wymagany typ dostepu do zasobu
                                  0,                               // typ udostepniania zasobu 0: nie mozna udostepnic portu COM
                                  nullptr,                         // ustawienia bezpieczenstwa SECURITY_ATTRIBUTES (brak)
                                  OPEN_EXISTING,                   // dzialanie do wykonania na zasobie ktory istnieje lub nie; dla urzadzen OPEN_EXISTING
                                  0,                               // operacje nakladajace sie "asynchroniczne"- tak
                                  nullptr                          // plik szablonu- brak dla portu COM
    );

    if (this->portHandle == INVALID_HANDLE_VALUE) throw "Blad uchwytu- port jest niedostepny\n";

    portSettings.DCBlength = sizeof(portSettings);   // dlugosc struktury w bajtach, ! musi byc ustawiona na sizeof(DCB)
    GetCommState(portHandle, &portSettings);         // Pobiera biezace ustawienia sterowania dla okreslonego urzadzenia komunikacyjnego.
    portSettings.BaudRate = CBR_9600;                // predkosc transmisji 9600 bps
    portSettings.Parity = NOPARITY;                  // brak bitu parzystosci
    portSettings.StopBits = ONESTOPBIT;              // ustawienie bitu stopu (jeden bit)
    portSettings.ByteSize = 8;                       // liczba wysylanych bitow

    portSettings.fParity = TRUE;                     // If this member is TRUE, parity checking is performed and errors are reported.
    portSettings.fDtrControl = DTR_CONTROL_DISABLE;  // Kontrola linii DTR: DTR_CONTROL_DISABLE (sygnal nieaktywny)
    portSettings.fRtsControl = RTS_CONTROL_DISABLE;  // Kontrola linii RTR: DTR_CONTROL_DISABLE (sygnal nieaktywny)
    portSettings.fOutxCtsFlow = FALSE;               // If this member is TRUE, the CTS (clear-to-send) signal is monitored for output flow control. If this member is TRUE and CTS is turned off, output is suspended until CTS is sent again.
    portSettings.fOutxDsrFlow = FALSE;               // If this member is TRUE, the DSR (data-set-ready) signal is monitored for output flow control. If this member is TRUE and DSR is turned off, output is suspended until DSR is sent again
    portSettings.fDsrSensitivity = FALSE;            // If this member is TRUE, the communications driver is sensitive to the state of the DSR signal. The driver ignores any bytes received, unless the DSR modem input line is high.
    portSettings.fAbortOnError = FALSE;              // If this member is TRUE, the driver terminates all read and write operations with an error status if an error occurs. The driver will not accept any further communications operations until the application has acknowledged the error by calling the ClearCommError function.
    portSettings.fOutX = FALSE;                      // Indicates whether XON/XOFF flow control is used during transmission. If this member is TRUE, transmission stops when the XoffChar character is received and starts again when the XonChar character is received.
    portSettings.fInX = FALSE;                       // Indicates whether XON/XOFF flow control is used during reception.
    portSettings.fErrorChar = FALSE;                 // Indicates whether bytes received with parity errors are replaced with the character specified by the ErrorChar member
    portSettings.fNull = FALSE;                      // If this member is TRUE, null bytes are discarded when received.
    SetCommState(portHandle, &portSettings);         // Configures a communications device according to the specifications in a device-control block

    portTimings.ReadIntervalTimeout = 3000;         // The maximum time allowed to elapse before the arrival of the next byte on the communications line, in milliseconds.
    portTimings.ReadTotalTimeoutConstant = 3000;    // A constant used to calculate the total time-out period for read operations, in milliseconds. For each read operation, this value is multiplied by the requested number of bytes to be read.
    portTimings.ReadTotalTimeoutMultiplier = 3000;  // A constant used to calculate the total time-out period for read operations, in milliseconds. For each read operation, this value is added to the product of the ReadTotalTimeoutMultiplier member and the requested number of bytes.
    portTimings.WriteTotalTimeoutMultiplier = 100;   // The multiplier used to calculate the total time-out period for write operations, in milliseconds. For each write operation, this value is multiplied by the number of bytes to be written.
    portTimings.WriteTotalTimeoutConstant = 100;     // A constant used to calculate the total time-out period for write operations, in milliseconds. For each write operation, this value is added to the product of the WriteTotalTimeoutMultiplier member and the number of bytes to be written.

    SetCommTimeouts(portHandle, &portTimings);       // Sets the time-out parameters for all read and write operations on a specified communications device.
    ClearCommError(portHandle, &portError, &portResources);  // Retrieves information about a communications error and reports the current status of a communications device.
}

void PortHandler::receive(const string& fileName) {
    BYTE type = 0;
    for (int i = 0; ; i++) {
        WriteFile(portHandle, &transmissionType, 1, &bitsLengthInChar, nullptr);         // Wysylamy sygnal gotowosci do odbioru
        cout << "Oczekiwanie na SOH" << '\n';
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);                               // pobranie 1 bajtu z zasobu
        cout << "Otrzymano odpowiedz: " << ((int) type) << '\n';
        if (SOH == type) {
            cout << "Ustalono polacznie z nadawca." << '\n';
            break;
        }
        if(i==100) {
            cout << "Nie ustanowono polacznia. Force EOT/" << '\n';
            exit(1);
        }
    }

    ofstream fout(fileName, ios::binary);
    cout << "Utworzono plik do odbioru, odczytywanie wiadomosci." << '\n';


    // Dopuki nie ma konca transmisji lub nie jest przerwana
    while(!(type == EOT || type == CAN)) {
        DataBlock dataBlock{};

        //odbieramy dane o rozmiarze pakietu bez pierwszego bajtu i wczytujemy je do niej
        ReadFile(portHandle, &dataBlock, numberOfBytesToRead-1, &dataBlockSize, nullptr);

        dataBlock.print();
        //czy bedziemy korzystac z CRC-16 (tryb pracy C || NAK)
        dataBlock.isCRC16 = (transmissionType == C);

        do {
            // Sprawdzamy poprawnosc- przy bledzie wysylamy NAK, gdy jest OK ACK
            if(!dataBlock.isCorrect()) {
                WriteFile(portHandle, &NAK, 1, &bitsLengthInChar, nullptr);
            } else {
                WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);
            }

            //odbieramy kolejny bajt naglowka
            ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
        } while(0 == type); //w przypadku zgubienia ACK/NAK wyslij jeszcze raz

        if(dataBlock.isCorrect()) {
            dataBlock.streamData(fout, EOT == type || CAN == type);
        }
    }

    if(CAN == type) {
        cout << "Wystapil blad przy przesylaniu" << '\n';
    }
    cout << "EOT FIN" << '\n';

    //potwierdzamy EOT
    WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);

    //sprawdzamy i potwierdzamy ETB
    ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
    if(ETB == type) {
        cout << "ETB FIN" << '\n';
        WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);
    }

    CloseHandle(portHandle);
    fout.close();
}

void PortHandler::send(const string& fileName) {
    BYTE type = 0;

    for (int i = 0; ; i++) {
        cout << "Oczekiwanie na C/NAK" << '\n';
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);             // pobranie 1 bajtu z zasobu
        cout << "Otrzymano odpowiedz: " << ((int) type) << '\n';
        if (C == type || NAK == type) {
            cout << "Ustalono polacznie z nadawca." << '\n';
            break;
        }
        if(i==100) {
            cout << "Nie ustanowono polacznia. Force EOT/" << '\n';
            exit(1);
        }
    }

    BYTE transmissionMethod = type;
    BYTE transmissionLength = C == type ? 133 : 132;
    ifstream is(fileName, ios::binary);
    deque<unsigned char> buffStream(istreambuf_iterator<char>(is), {});
    is.close();

    BYTE packetNo = 1;
    int globalPacket = 0;
    while(!buffStream.empty()) {
        cout << "-=-=-=-=-=-=-=-=-=-=- " << globalPacket << "-=-=-=-=-=-=-=-=-=-=-" << '\n';
        cout << "Rozpoczynam komponowanie pakietu" << '\n';
        BYTE tmpType = 0;
        DataBlock dataBlock{};
        dataBlock.packetNoQue = packetNo;
        dataBlock.packetNoNeg = 0xFF - packetNo;
        dataBlock.isCRC16 = (C == transmissionMethod);

        for(int i=0; i< (sizeof(dataBlock.data)/ sizeof(dataBlock.data[0])); i++) {
            if(buffStream.empty()) {
                dataBlock.data[i] = 26;
            } else {
                dataBlock.data[i] = buffStream.front();
                buffStream.pop_front();
            }
        }
        dataBlock.generateCRC();

        cout << "Wygenerowano dane pakietu" << '\n';

        do {
            // wyslij naglowek oraz blok danych
            WriteFile(portHandle, &SOH, 1, &bitsLengthInChar, nullptr);
            cout << "Przeslano naglowek SOH pakietu" << '\n';
            WriteFile(portHandle, &dataBlock, transmissionLength-1, &bitsLengthInChar, nullptr);
            cout << "Przeslano dane pakietu" << '\n';

            // odczytaj informacje zwrotna
            ReadFile(portHandle, &tmpType, 1, &bitsLengthInChar, nullptr);
            cout << "Odczytano wartosc tmpType: " << (int) tmpType << '\n';

            if(ACK == tmpType) {
                cout << "Wyslano pakiet" << '\n';
                break;
            } else if(NAK == tmpType) {
                cout << "Blad wysylania pakietu, ponawianie" << '\n';
                continue;
            } else if(CAN == tmpType) {
                cout << "Polaczenie przerwane" << '\n';
                exit(1);
            }

        } while(0 != tmpType);

        packetNo++;
        globalPacket++;
    }

    cout << '\n' << "Przeslano wszystkie dane; przesylanie EOT" << '\n';
    do {
        WriteFile(portHandle, &EOT, 1, &bitsLengthInChar, nullptr);
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
        if(ACK == type) {
            cout << "Potwierdzono zakonczenie transmisji" << '\n';
        } else {
            cout << "Wystapil blad przy konczeniu transmisji, ponawianie" << '\n';
        }
    } while (0 == type);

    CloseHandle(portHandle);
}