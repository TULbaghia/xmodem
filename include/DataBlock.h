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
        for(unsigned char i : data) {
            cout << (int) i << "-";
        }
        cout << '\n' << (int)CRC[0] << "__" << (int)CRC[1] << '\n';
        cout << "-=-=-=" << '\n';
    }

    void streamData(ofstream& ofs) {
        for(int i=0; i<sizeof(data)/sizeof(data[0]); i++) {
            ofs << data[i];
        }
    }

    bool isCorrect() {
        return isCorrectNo() && isCorrectCheckSum();
    }

    bool isCorrectNo() {
        if(255 != packetNoNeg + packetNoQue) {
            cout << "Niepoprawny numer pakietu." << '\n';
            return false;
        }
        return true;
    }

    bool isCorrectCheckSum() {
        if(calculateDataCRC() != calculateBlockCRC()) {
            cout << "Niepoprawna suma kontrolna." << '\n';
            return false;
        }
        return true;
    }

    int calculateDataCRC() {
        int count = sizeof(data) / sizeof(data[0]);
        BYTE *ptr = data;
        if(isCRC16) {
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

            while(count--) {
                checksum += (*ptr++);
            }
            checksum %= 256;
            return checksum;
        }
    }

    int calculateBlockCRC() {
        return isCRC16 ? (CRC[0] << 8) | CRC[1] & 0x00FF : CRC[0];
    }
};

//rozmiar bloku bez isCRC16
unsigned long dataBlockSize = sizeof(DataBlock) - sizeof(bool);

#endif //XMODEM_DATABLOCK_H
