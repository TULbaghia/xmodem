#ifndef XMODEM_DATABLOCK_H
#define XMODEM_DATABLOCK_H

#include <iostream>
#include <fstream>

using std::cout;
using std::ofstream;

struct DataBlock {
    BYTE packetNoQue;   // 0..255
    BYTE packetNoNeg;   // 255..0
    BYTE data[128];     // przesylane dane
    BYTE CRC[2];        // suma kontrolna przeslanych danych
    bool isCRC16;       // blok uzywa CRC16 lub Algebraiczna Sumy Kontrolnej (ASK)

    void print() {
        cout << "-=-=-=" << '\n';
        cout << (int) packetNoQue << "___" << (int) packetNoNeg << '\n';
        for (unsigned char i : data) {
            cout << (int) i << "-";
        }
        cout << '\n' << (int) CRC[0] << "__" << (int) CRC[1] << '\n';
        cout << "-=-=-=" << '\n';
    }

    void streamData(ofstream &ofstr, bool isLastPacket) {
        int skipAfter = 0;
        // jezeli ostatni pakiet zlicz ile bajtow paddingu zostalo dodanych
        if (isLastPacket) {
            for (int i = sizeof(data) / sizeof(data[0]) - 1; i >= 0; i--) {
                if (data[i] == 0x1A) { // padding (26)
                    skipAfter++;
                }
            }
        }
        // i mniejsze od dlugosci przeslanych danych pomniejszonych o dlugosc paddingu
        for (int i = 0; i < (sizeof(data) / sizeof(data[0])) - skipAfter; i++) {
            ofstr << data[i];
        }
    }

    bool isCorrect() {
        return isCorrectNo() && isCorrectChecksum();
    }

    // Sprawdza czy numer pakietu i jego dopelnienie daje 0xFF
    bool isCorrectNo() {
        if (255 != packetNoNeg + packetNoQue) {
            cout << "Niepoprawny numer pakietu." << '\n';
            return false;
        }
        return true;
    }

    // Sprawdza poprawnosc sumy kontrolnej, porownujac sume kontrolna bloku danych z przeslana suma CRC | ASK
    bool isCorrectChecksum() {
        if (calculateChecksum() != calculateBlockCRC()) {
            cout << "Niepoprawna suma kontrolna." << '\n';
            return false;
        }
        return true;
    }

    // przelicza sume kontrolna CRC16 lub Algebraiczna Sume Kontrolna
    int calculateChecksum() {
        int count = sizeof(data) / sizeof(data[0]); // dlugosc tablicy dane
        BYTE *ptr = data; // wsk na pierwszy element tablicy

        if (isCRC16) {                              // jezeli CRC to:
            int crc = 0;
            while (count--) {
                crc ^= (*ptr++ << 8);               // XOR starszej czesci crc z nowa liczba przesunieta do starszych bitow
                for (int i = 0; i < 8; i++) {
                    if (crc & 0x8000) {             // sprawdzenie czy najstarszy bit jest rowny 1, nie chcemy go stracic
                        crc = (crc << 1) ^ 0x1021;  // wyrzucenie MSB oraz dodanie 0 jako LSB oraz XOR z generatorem (Most/Lease Significant Bit)
                    } else {
                        crc <<= 1;                  // wyrzucenie MSB oraz dodanie 0 jako LSB
                    }
                }
            }
            return (crc & 0xFFFF);
        } else {                                    // jezeli ASK to:
            int checksum = 0;

            while (count--) {
                checksum += (*ptr++);               // sumuj wszystkie wartosci z dane
            }
            checksum %= 256;                        // modulo 256
            return checksum;
        }
    }

    int calculateBlockCRC() {
        // stworzenie liczby 16 bitowej z dwuch blokow 8 bit jako CRC[0]_CRC[1]
        return isCRC16 ? (CRC[0] << 8) | CRC[1] & 0x00FF : CRC[0];
    }

    void generateCRC() {
        if (isCRC16) {
            int crc = calculateChecksum();
            CRC[0] = (BYTE) ((crc >> 8) & 0xff);     // pobranie 8 bitow ze starszych wartosci sumy CRC
            CRC[1] = (BYTE) (crc & 0xff);            // pobranie 8 bitow z mlodszych wartosci sumy CRC
        } else {
            CRC[0] = (BYTE) calculateChecksum();    // przypisanie 8 bitow Algebraicznej sumy kontrolnej
        }
    }
};

//rozmiar bloku bez isCRC16
unsigned long dataBlockSize = sizeof(DataBlock) - sizeof(bool);

#endif //XMODEM_DATABLOCK_H
