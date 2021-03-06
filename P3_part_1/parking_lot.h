#ifndef C_TESTING2_PARKING_LOT_H
#define C_TESTING2_PARKING_LOT_H

typedef struct space_s {
    int type = 0;
    int vacant = 1;
    int car = 0;
    int y = 0;
    int m = 0;
    int d = 0;
} space;

class Parking_lot {
public:
    Parking_lot();

    ~Parking_lot();

    void arrival(int type, int arrival_y, int arrival_m, int arrival_d, int num);

    void departure(int num, int arrival_y, int arrival_m, int arrival_d);

private:
    space *space1;
};

#endif //C_TESTING2_PARKING_LOT_H
