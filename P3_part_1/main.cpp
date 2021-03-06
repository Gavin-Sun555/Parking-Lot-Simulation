#include <iostream>
#include "parking_lot.h"

using namespace std;

int main() {
    int choose, y, m, d, t, num;
    cout << "Please type 1 for arrival or 0 for departure!(-1 to exit)";
    cin >> choose;
    cout << endl;
    while (choose != -1) {
        if (choose == 1) {
            cout << "Please give the date today.Year: ";
            cin >> y;
            cout << " Month: ";
            cin >> m;
            cout << "Day: ";
            cin >> d;
            cout << endl;
            cout << "Please choose the type of your car. Van: 1; Car: 2; Motorbike: 3; Bicycle: 4.";
            cin >> t;
            cout << endl;
            cout << "Please give me your car number: ";
            cin >> num;
            Parking_lot p;
            p.arrival(t, y, m, d, num);
        } else {
            cout << "Please give the date today.Year: ";
            cin >> y;
            cout << " Month: ";
            cin >> m;
            cout << "Day: ";
            cin >> d;
            cout << endl;
            cout << "Please give me your car number: ";
            cin >> num;
            Parking_lot p;
            p.departure(num, y, m, d);
        }
        cout << "Please type 1 for arrival or 0 for departure!(-1 to exit)";
        cin >> choose;
        cout << endl;
    }

    return 0;
}