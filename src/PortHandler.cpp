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

PortHandler::PortHandler(const string &portName, const string &transmissionMode) : transmissionType(
        transmissionMode == "C" ? C : NAK), numberOfBytesToRead(transmissionMode == "C" ? 133 : 132) {
    this->portHandle = CreateFile(portName.c_str(),                // Nazwa zasobu ktory ma zostac uzyty
                                  GENERIC_READ | GENERIC_WRITE,    // wymagany typ dostepu do zasobu
                                  0,                               // typ udostepniania zasobu 0: nie mozna udostepnic portu COM
                                  nullptr,                         // ustawienia bezpieczenstwa SECURITY_ATTRIBUTES (brak)
                                  OPEN_EXISTING,                   // dzialanie do wykonania na zasobie ktory istnieje lub nie; dla urzadzen OPEN_EXISTING
                                  0,                               // operacje nakladajace sie "asynchroniczne"- tak
                                  nullptr                          // plik szablonu- brak dla portu COM
    );

    if (this->portHandle == INVALID_HANDLE_VALUE) throw "Blad uchwytu- port jest niedostepny\n";

    portSettings.DCBlength = sizeof(portSettings);           // dlugosc struktury w bajtach, ! musi byc ustawiona na sizeof(DCB)
    GetCommState(portHandle, &portSettings);                 // Pobiera biezace ustawienia sterowania dla okreslonego urzadzenia komunikacyjnego.

    portSettings.BaudRate = CBR_9600;                        // predkosc transmisji 9600 bps
    portSettings.Parity = NOPARITY;                          // brak bitu parzystosci
    portSettings.StopBits = ONESTOPBIT;                      // ustawienie bitu stopu (jeden bit)
    portSettings.ByteSize = 8;                               // liczba wysylanych bitow

    portSettings.fParity = TRUE;                             // wykonuj testy poprawnosci i zglaszaj bledy
    portSettings.fDtrControl = DTR_CONTROL_DISABLE;          // Kontrola data-terminal-ready DTE (np. komputer)
    portSettings.fRtsControl = RTS_CONTROL_DISABLE;          // Kontrola request-to-send ramka kontrolna (MAC) w wartstwie dostepu do danych
    portSettings.fOutxCtsFlow = FALSE;                       // Monitoruj clear-to-send
    portSettings.fOutxDsrFlow = FALSE;                       // Monitoruj data-set-ready gotowe do wyslania lub odebrania danych
    portSettings.fDsrSensitivity = FALSE;                    // aktywuj tylko na wysoki stan od modemu
    portSettings.fAbortOnError = FALSE;                      // zakoncz wszystkie operacje IO ze statusem bledu gdy taki wystapi
    portSettings.fOutX = FALSE;                              // zatrzymuj transmisje przy znaku XON / XOFF
    portSettings.fInX = FALSE;                               // jak ma zachowac sie XON / XOFF w buforze
    portSettings.fErrorChar = FALSE;                         // Wskazuje, czy bajty odebrane z błędami parzystości są zastępowane znakiem określonym przez element ErrorChar
    portSettings.fNull = FALSE;                              // Odrzucac NULL bajty
    SetCommState(portHandle, &portSettings);                 // konfiguruj urzadzenie komunikacyjne

    portTimings.ReadIntervalTimeout = 3000;                  // Maksymalny czas, jaki upływa przed przybyciem następnego bajtu na linię komunikacyjną, w milisekundach.
    portTimings.ReadTotalTimeoutConstant = 3000;             // Stała używana do obliczania całkowitego limitu czasu operacji odczytu w milisekundach. Dla każdej operacji odczytu wartość ta jest mnożona przez wymaganą liczbę bajtów do odczytania.
    portTimings.ReadTotalTimeoutMultiplier = 3000;           // Stała używana do obliczania całkowitego limitu czasu operacji odczytu w milisekundach. Dla każdej operacji odczytu wartość ta jest dodawana do iloczynu elementu ReadTotalTimeoutMultiplier i żądanej liczby bajtów.
    portTimings.WriteTotalTimeoutMultiplier = 100;           // Mnożnik używany do obliczania całkowitego limitu czasu operacji zapisu w milisekundach. Dla każdej operacji zapisu wartość ta jest mnożona przez liczbę bajtów do zapisania.
    portTimings.WriteTotalTimeoutConstant = 100;             // Stała używana do obliczania całkowitego limitu czasu operacji zapisu w milisekundach. Dla każdej operacji zapisu wartość ta jest dodawana do iloczynu elementu WriteTotalTimeoutMultiplier i liczby bajtów do zapisania.

    SetCommTimeouts(portHandle, &portTimings);               // Ustawia parametry limitu czasu dla wszystkich operacji odczytu i zapisu na określonym urządzeniu komunikacyjnym.
    ClearCommError(portHandle, &portError, &portResources);  // Pobiera informacje o błędzie komunikacji i zgłasza bieżący stan urządzenia komunikacyjnego.
}

void PortHandler::receive(const string &fileName) {
    BYTE type = 0;
    for (int i = 0;; i++) {
        WriteFile(portHandle, &transmissionType, 1, &bitsLengthInChar, nullptr);         // Wysylamy sygnal gotowosci do odbioru
        cout << "Oczekiwanie na SOH" << '\n';
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);                               // pobranie 1 bajtu z zasobu
        cout << "Otrzymano odpowiedz: " << ((int) type) << '\n';
        if (SOH == type) {                                                                                                         // jezeli bajt SOH to rozpoczynamy transmisje
            cout << "Ustalono polacznie z nadawca." << '\n';
            break;
        }
        if (i == 100) {
            cout << "Nie ustanowono polacznia. Force EOT/" << '\n';
            exit(1);
        }
    }

    ofstream fout(fileName, ios::binary);
    cout << "Utworzono plik do odbioru, odczytywanie wiadomosci." << '\n';


    // Dopuki nie ma konca transmisji lub nie jest przerwana
    while (!(type == EOT || type == CAN)) {
        DataBlock dataBlock{};

        //odbieramy dane o rozmiarze pakietu bez pierwszego bajtu i wczytujemy je do niej
        ReadFile(portHandle, &dataBlock, numberOfBytesToRead - 1, &dataBlockSize, nullptr);

        dataBlock.print();
        //czy bedziemy korzystac z CRC-16 (tryb pracy C || NAK)
        dataBlock.isCRC16 = (transmissionType == C);

        do {
            // Sprawdzamy poprawnosc- przy bledzie wysylamy NAK, gdy jest OK ACK
            if (!dataBlock.isCorrect()) {
                WriteFile(portHandle, &NAK, 1, &bitsLengthInChar, nullptr);
            } else {
                WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);
            }

            //odbieramy kolejny bajt naglowka
            ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
        } while (0 == type); //w przypadku zgubienia ACK/NAK wyslij jeszcze raz

        if (dataBlock.isCorrect()) {
            dataBlock.streamData(fout, EOT == type || CAN == type);
        }
    }

    if (CAN == type) {
        cout << "Wystapil blad przy przesylaniu" << '\n';
    }
    cout << "EOT FIN" << '\n';

    //potwierdzamy EOT
    WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);

    //sprawdzamy i potwierdzamy ETB
    ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
    if (ETB == type) {
        cout << "ETB FIN" << '\n';
        WriteFile(portHandle, &ACK, 1, &bitsLengthInChar, nullptr);
    }

    CloseHandle(portHandle);
    fout.close();
}

void PortHandler::send(const string &fileName) {
    BYTE type = 0;

    for (int i = 0; ; i++) {
        cout << "Oczekiwanie na C/NAK" << '\n';
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);             // oczekiwanie na probe nadania danych
        cout << "Otrzymano odpowiedz: " << ((int) type) << '\n';
        if (C == type || NAK == type) {                                                                          // jezeli C lub ASK rozpoczynamy transmisje
            cout << "Ustalono polacznie z nadawca." << '\n';
            break;
        }
        if(i==100) {
            cout << "Nie ustanowono polacznia. Force EOT/" << '\n';
            exit(1);
        }
    }

    BYTE transmissionMethod = type;                                                                              // zapisujemy metode transmisji C | ASK
    BYTE transmissionLength = C == type ? 133 : 132;                                                             // ustalamy ilosc bajtow dla typu transmisji
    ifstream is(fileName, ios::binary);
    deque<unsigned char> buffStream(istreambuf_iterator<char>(is), {});                                      // wczytujemy wartosci ze strumienie do deque
    is.close();

    BYTE packetNo = 1;
    int globalPacket = 0;
    while(!buffStream.empty()) {                                                                                // jezeli mamy dane na liscie
        cout << "-=-=-=-=-=-=-=-=-=-=- " << globalPacket << "-=-=-=-=-=-=-=-=-=-=-" << '\n';
        cout << "Rozpoczynam komponowanie pakietu" << '\n';
        BYTE tmpType = 0;
        DataBlock dataBlock{};
        dataBlock.packetNoQue = packetNo;                                                                       // ustawiamy numer pakietu
        dataBlock.packetNoNeg = 0xFF - packetNo;                                                                // wyliczamy dopelnienie
        dataBlock.isCRC16 = (C == transmissionMethod);                                                          // czy otrzymano C

        for(int i=0; i< (sizeof(dataBlock.data)/ sizeof(dataBlock.data[0])); i++) {
            if(buffStream.empty()) {                                                                            // jezeli lista jest pusta dopelnij blok wartosciami 26 (0x1A)
                dataBlock.data[i] = 26;
            } else {
                dataBlock.data[i] = buffStream.front();                                                         // odczytaj wartosc z poczatka listy i przypisz do bloku danych
                buffStream.pop_front();                                                                         // usun wartosc z poczatku listy
            }
        }
        dataBlock.generateCRC();                                                                                // generuj CRC dla bloku danych

        cout << "Wygenerowano dane pakietu" << '\n';

        do {
            // wyslij naglowek oraz blok danych
            WriteFile(portHandle, &SOH, 1, &bitsLengthInChar, nullptr);
            cout << "Przeslano naglowek SOH pakietu" << '\n';
            WriteFile(portHandle, &dataBlock, transmissionLength - 1, &bitsLengthInChar, nullptr);
            cout << "Przeslano dane pakietu" << '\n';

            // odczytaj informacje zwrotna
            ReadFile(portHandle, &tmpType, 1, &bitsLengthInChar, nullptr);
            cout << "Odczytano wartosc tmpType: " << (int) tmpType << '\n';

            if (ACK == tmpType) {
                cout << "Wyslano pakiet" << '\n';
                break;
            } else if (NAK == tmpType) {
                cout << "Blad wysylania pakietu, ponawianie" << '\n';
                continue;
            } else if (CAN == tmpType) {
                cout << "Polaczenie przerwane" << '\n';
                exit(1);
            }

        } while (0 != tmpType);

        packetNo++;
        globalPacket++;
    }

    cout << '\n' << "Przeslano wszystkie dane; przesylanie EOT" << '\n';
    do {
        WriteFile(portHandle, &EOT, 1, &bitsLengthInChar, nullptr);
        ReadFile(portHandle, &type, 1, &bitsLengthInChar, nullptr);
        if (ACK == type) {
            cout << "Potwierdzono zakonczenie transmisji" << '\n';
        } else {
            cout << "Wystapil blad przy konczeniu transmisji, ponawianie" << '\n';
        }
    } while (0 == type);

    CloseHandle(portHandle);
}
