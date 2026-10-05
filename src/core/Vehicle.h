#ifndef VEHICLE_H
#define VEHICLE_H

#include <string>
#include <ostream>

enum class VehicleType {
    VAN = 1,
    CAR = 2,
    MOTORBIKE = 3,
    BICYCLE = 4
};

inline std::string vehicleTypeName(VehicleType type) {
    switch (type) {
        case VehicleType::VAN: return "van";
        case VehicleType::CAR: return "car";
        case VehicleType::MOTORBIKE: return "motorbike";
        case VehicleType::BICYCLE: return "bicycle";
        default: return "unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, VehicleType type) {
    return os << vehicleTypeName(type);
}


inline double vehicleDailyRate(VehicleType type) {
    switch (type) {
        case VehicleType::VAN: return 20.0;
        case VehicleType::CAR: return 15.0;
        case VehicleType::MOTORBIKE: return 10.0;
        case VehicleType::BICYCLE: return 5.0;
        default: return 10.0;
    }
}

struct Date {
    int year = 2026;
    int month = 10;
    int day = 4;

    int totalDays() const {
        return day + 30 * month + 365 * year;
    }

    static int diffDays(const Date& arrival, const Date& departure) {
        int diff = departure.totalDays() - arrival.totalDays();
        return diff >= 0 ? diff : 0;
    }
};

struct Vehicle {
    int id = 0;
    VehicleType type = VehicleType::CAR;
    Date arrivalDate;
    int floor = 1;
    int spaceNumber = 0;
    bool active = false;
};

#endif // VEHICLE_H
