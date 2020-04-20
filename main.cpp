#include <iostream>
#include <PortHandler.h>

using namespace std;

string setWorkMode(string &TRANSMISSION_MODE);

string setPortListener();

int main() {
    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-= xModem -=-=-=-=-=-=-=-=-=-=-=-=-=-=" << '\n';
    string TRANSMISSION_MODE;
    string WORKING_MODE = setWorkMode(TRANSMISSION_MODE);
    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
    string PORT_LISTENER = setPortListener();
    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';
    cout << "Wybrano tryb pracy: " << WORKING_MODE << '\n';
    cout << "Nasluchiwanie na porcie: " << PORT_LISTENER << '\n';
    cout << "Tryb pracy: " << TRANSMISSION_MODE << '\n';
    cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << '\n';

    string FILE_NAME;
    cout << "Podaj nazwe pliku, ktorego chcesz uzyc: resources/";
    cin >> FILE_NAME;

    PortHandler portHandler(PORT_LISTENER, TRANSMISSION_MODE);

    if ("RECEIVER" == WORKING_MODE) {
        portHandler.receive("./resources/" + FILE_NAME);
    } else {
        portHandler.send("./resources/" + FILE_NAME);
    }

    return 0;
}

string setPortListener() {
    int tmp;
    cout << "Format: Wpisz liczbe." << '\n';
    cout << "Wybierz port do nasluchiwania: COM";
    cin >> tmp;
    return "COM" + to_string(tmp);
}

string setWorkMode(string &TRANSMISSION_MODE) {
    cout << "Wybierz tryb pracy: " << '\n';
    cout << "1) Nadawca (C)" << '\n';
    cout << "2) Nadawca (NAK)" << '\n';
    cout << "3) Odbierca (C)" << '\n';
    cout << "4) Odbierca (NAK)" << '\n';
    cout << "-=-=-=-=-" << '\n';
    while (true) {
        int tmp;
        cout << "Pracuj jako:";
        cin >> tmp;

        if (1 == tmp || 3 == tmp) TRANSMISSION_MODE = "C";
        else if (2 == tmp || 4 == tmp) TRANSMISSION_MODE = "NAK";

        if (1 == tmp || 2 == tmp) return "TRANSMITTER";
        else if (3 == tmp || 4 == tmp) return "RECEIVER";
        else cout << "Nieobslugiwany tryb pracy." << '\n';
    }
}
