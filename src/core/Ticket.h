#ifndef TICKET_H
#define TICKET_H

#include <string>
#include <sstream>
#include <iomanip>
#include "Vehicle.h"

struct ArrivalTicket {
    int vehicleId = 0;
    VehicleType vehicleType = VehicleType::CAR;
    Date arrivalDate;
    int floor = 1;
    int spaceNumber = 0;

    std::string toString() const {
        std::ostringstream oss;
        oss << "========================================\n"
            << "            ARRIVAL TICKET              \n"
            << "========================================\n"
            << "Vehicle ID:          " << vehicleId << "\n"
            << "Vehicle Type:        " << vehicleTypeName(vehicleType) << "\n"
            << "Time of Arrival:     " << arrivalDate.month << "/" << arrivalDate.day << "/" << arrivalDate.year << "\n"
            << "Allocated Spot:      Space " << (spaceNumber + 1) << "\n"
            << "Daily Rate:          $" << std::fixed << std::setprecision(2) << vehicleDailyRate(vehicleType) << "\n"
            << "========================================\n";
        return oss.str();
    }
};

struct DepartureTicket {
    int vehicleId = 0;
    VehicleType vehicleType = VehicleType::CAR;
    Date arrivalDate;
    Date departureDate;
    int yearsSpent = 0;
    int monthsSpent = 0;
    int daysSpent = 0;
    int totalDays = 0;
    double price = 0.0;
    int floor = 1;
    int spaceNumber = 0;

    std::string toString() const {
        std::ostringstream oss;
        oss << "========================================\n"
            << "           DEPARTURE TICKET             \n"
            << "========================================\n"
            << "Vehicle ID:          " << vehicleId << "\n"
            << "Vehicle Type:        " << vehicleTypeName(vehicleType) << "\n"
            << "Time of Arrival:     " << arrivalDate.month << "/" << arrivalDate.day << "/" << arrivalDate.year << "\n"
            << "Time of Departure:   " << departureDate.month << "/" << departureDate.day << "/" << departureDate.year << "\n"
            << "Duration:            " << yearsSpent << " years, " << monthsSpent << " months, " << daysSpent << " days (" << totalDays << " total days)\n"
            << "Spot Released:       Space " << (spaceNumber + 1) << "\n"
            << "Total Price:         $" << std::fixed << std::setprecision(2) << price << "\n"
            << "========================================\n";
        return oss.str();
    }
};

#endif // TICKET_H
