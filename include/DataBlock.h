#ifndef XMODEM_DATABLOCK_H
#define XMODEM_DATABLOCK_H

#include <iostream>
#include <fstream>

using std::cout;
using std::ofstream;

struct DataBlock {
    BYTE packetNoQue;   // 0..255
    BYTE packetNoNeg;   // 255..0 -> 255 - packetNoQue = packetNoNeg
    BYTE data[128];
    BYTE CRC[2];
    bool isCRC16;

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
        if (isLastPacket) {
            for (int i = sizeof(data) / sizeof(data[0]) - 1; i >= 0; i--) {
                if (data[i] == 0x1A) { // padding (26)
                    skipAfter++;
                }
            }
        }
        for (int i = 0; i < (sizeof(data) / sizeof(data[0])) - skipAfter; i++) {
            ofstr << data[i];
        }
    }

    bool isCorrect() {
        return isCorrectNo() && isCorrectChecksum();
    }

    bool isCorrectNo() {
        if (255 != packetNoNeg + packetNoQue) {
            cout << "Niepoprawny numer pakietu." << '\n';
            return false;
        }
        return true;
    }

    bool isCorrectChecksum() {
        if (calculateChecksum() != calculateBlockCRC()) {
            cout << "Niepoprawna suma kontrolna." << '\n';
            return false;
        }
        return true;
    }

    int calculateChecksum() {
        int count = sizeof(data) / sizeof(data[0]);
        BYTE *ptr = data;
        if (isCRC16) {
            int crc = 0;
            while (count--) {
                crc ^= (*ptr++ << 8);               // XOR starszej czesci crc z nowa liczba przesynieta to starszych wartosci
                for (int i = 0; i < 8; i++) {
                    if (crc & 0x8000)               // sprawdzenie czy najstarszy bit jest rowny 1, nie chcemy go stracic
                        crc = (crc << 1) ^ 0x1021;  // XOR z generatorem
                    else
                        crc <<= 1;             // pomnozenie o 2
                }
            }
            return (crc & 0xFFFF);
        } else {
            int checksum = 0;

            while (count--) {
                checksum += (*ptr++);
            }
            checksum %= 256;
            return checksum;
        }
    }

    int calculateBlockCRC() {
        return isCRC16 ? (CRC[0] << 8) | CRC[1] & 0x00FF : CRC[0];
    }

    void generateCRC() {
        if(isCRC16) {
            int crc = calculateChecksum();
            CRC[0] = (BYTE)((crc >> 8) & 0xff);
            CRC[1] = (BYTE)(crc & 0xff);
        } else {
            CRC[0] = (BYTE) calculateChecksum();
        }
    }
};

//rozmiar bloku bez isCRC16
unsigned long dataBlockSize = sizeof(DataBlock) - sizeof(bool);

#endif //XMODEM_DATABLOCK_H
