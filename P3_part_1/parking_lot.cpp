#include <iostream>
#include "parking_lot.h"

using namespace std;

Parking_lot::Parking_lot() {
    space1 = new space[20];
}

Parking_lot::~Parking_lot() {
    for (int i = 0; i < 20; i++) {
        delete[] space1;
    }
}

void Parking_lot::arrival(int type, int arrival_y, int arrival_m, int arrival_d, int num) {
    cout << "type: " << type << " y: " << arrival_y << endl;
    switch (type) {
        case 1:
            for (int i = 0; i < 5; i++) {
                if (space1[i].vacant == 1) {
                    space1[i].vacant = 0;
                    space1[i].car = num;
                    space1[i].type = 1;
                    space1[i].y = arrival_y;
                    space1[i].m = arrival_m;
                    space1[i].d = arrival_d;
                    cout << "ARRIVAL TICKET" << endl;
                    cout << "Time of arrival: " << arrival_m << "/" << arrival_d << "/" << arrival_y << endl;
                    cout << "Type of the vehicle: van" << endl;
                    cout << "Floor 2, space number " << i << endl;
                    break;
                }
            }
            break;
        case 2:
            for (int i = 0; i < 5; i++) {
                if (space1[5 + i].vacant == 1) {
                    space1[5 + i].vacant = 0;
                    space1[5 + i].car = num;
                    space1[i].type = 2;
                    space1[5 + i].y = arrival_y;
                    space1[5 + i].m = arrival_m;
                    space1[5 + i].d = arrival_d;
                    cout << "ARRIVAL TICKET" << endl;
                    cout << "Time of arrival: " << arrival_m << "/" << arrival_d << "/" << arrival_y << endl;
                    cout << "Type of the vehicle: car" << endl;
                    cout << "Floor 2, space number " << i << endl;
                    break;
                }
            }
            break;
        case 3:
            for (int i = 0; i < 5; i++) {
                if (space1[10 + i].vacant == 1) {
                    space1[10 + i].vacant = 0;
                    space1[10 + i].car = num;
                    space1[i].type = 3;
                    space1[10 + i].y = arrival_y;
                    space1[10 + i].m = arrival_m;
                    space1[10 + i].d = arrival_d;
                    cout << "ARRIVAL TICKET" << endl;
                    cout << "Time of arrival: " << arrival_m << "/" << arrival_d << "/" << arrival_y << endl;
                    cout << "Type of the vehicle: motorbike" << endl;
                    cout << "Floor 1, space number " << i << endl;
                    break;
                }
            }
            break;
        case 4:
            for (int i = 0; i < 5; i++) {
                if (space1[15 + i].vacant == 1) {
                    space1[15 + i].vacant = 0;
                    space1[15 + i].car = num;
                    space1[i].type = 4;
                    space1[15 + i].y = arrival_y;
                    space1[15 + i].m = arrival_m;
                    space1[15 + i].d = arrival_d;
                    cout << "ARRIVAL TICKET" << endl;
                    cout << "Time of arrival: " << arrival_m << "/" << arrival_d << "/" << arrival_y << endl;
                    cout << "Type of the vehicle: bike" << endl;
                    cout << "Floor 1, space number " << i << endl;
                    break;
                }
            }
            break;
        default:
            cout << "Wrong Type!" << endl;
    }
}

void Parking_lot::departure(int num, int departure_y, int departure_m, int departure_d) {
    int y, m, d;
    for (int i = 0; i < 20; i++) {
        if (space1[i].car == num) {
            y = departure_y - space1[i].y;
            m = departure_m - space1[i].m;
            d = departure_d - space1[i].d;
            cout << "DEPARTURE TICKET" << endl;
            cout << "Time spent in the parking lot: " << y << " years, " << m << " months, " << d << " days" << endl;
            if (space1[i].type == 1) {
                cout << "Type of the vehicle: van" << endl;
                cout << "Price: $" << 20 * (d + 30 * m + 365 * y) << endl;
            } else if (space1[i].type == 2) {
                cout << "Type of the vehicle: car" << endl;
                cout << "Price: $" << 15 * (d + 30 * m + 365 * y) << endl;
            } else if (space1[i].type == 3) {
                cout << "Type of the vehicle: motorbike" << endl;
                cout << "Price: $" << 10 * (d + 30 * m + 365 * y) << endl;
            } else if (space1[i].type == 4) {
                cout << "Type of the vehicle: bicycle" << endl;
                cout << "Price: $" << 5 * (d + 30 * m + 365 * y) << endl;
            }
            break;
        }
    }
}

