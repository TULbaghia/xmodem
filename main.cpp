#include <iostream>
#include <FileHandler/PortHandler.h>

using namespace std;

string setWorkMode();

string setPortListener();

string setTransmissionMode();

int main() {

    PortHandler portHandler("COM2", "C");

    portHandler.receive("./resources/receive.txt");
    exit(1);

//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-= xModem -=-=-=-=-=-=-=-=-=-=-=-=-=-=" << '\n';
//    string WORKING_MODE = setWorkMode();
//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
//    string PORT_LISTENER = setPortListener();
//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
//    string TRANSMISSION_MODE = setTransmissionMode();
//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
//    cout << "Wybrano tryb pracy: " << WORKING_MODE << '\n';
//    cout << "Nasluchiwanie na porcie: " << PORT_LISTENER << '\n';
//    cout << "Tryb pracy: " << TRANSMISSION_MODE << '\n';
//    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
//
//    PortHandler portHandler(PORT_LISTENER, TRANSMISSION_MODE);
//
//    portHandler.receive();
//
//    return 0;
}

string setTransmissionMode() {
    cout << "Wybierz tryb transmisji: " << '\n';
    cout << "1) Algebraiczna suma kontrolna (NAK)" << '\n';
    cout << "2) CRC (C)" << '\n';
    while (true) {
        int tmp;
        cout << "Pracuj jako: ";
        cin >> tmp;
        if (1 == tmp || 2 == tmp) return 1 == tmp ? "NAK" : "C";
        else cout << "Nieobslugiwany tryb transmisji." << '\n';
    }
}

string setPortListener() {
    int tmp;
    cout << "Format: Wpisz liczbe." << '\n';
    cout << "Wybierz port do nasluchiwania: COM" << '\n';
    cin >> tmp;
    return "COM" + to_string(tmp);
}

string setWorkMode() {
    cout << "Wybierz tryb pracy: " << '\n';
    cout << "1) Nadawca" << '\n';
    cout << "2) Odbierca" << '\n';
    cout << "-=-=-=-=-" << '\n';
    while (true) {
        int tmp;
        cout << "Pracuj jako: ";
        cin >> tmp;
        if (1 == tmp) return "TRANSMITTER";
        else if (2 == tmp) return "RECEIVER";
        else cout << "Nieobslugiwany tryb pracy." << '\n';
    }
}
