#include "ParkingLot.h"
#include <iostream>
#include <iomanip>

ParkingLot::ParkingLot() {
    spots_.resize(TOTAL_SPOTS);

    // Initialize 10 spots on a single floor
    // Stalls 0-1: Motorbike
    // Stalls 2-3: Bicycle
    // Stalls 4-7: Car
    // Stalls 8-9: Van
    for (int i = 0; i < TOTAL_SPOTS; ++i) {
        spots_[i].floor = 1;
        spots_[i].spaceNumber = i;
        spots_[i].vacant = true;

        if (i < 2) {
            spots_[i].preferredType = VehicleType::MOTORBIKE;
        } else if (i < 4) {
            spots_[i].preferredType = VehicleType::BICYCLE;
        } else if (i < 8) {
            spots_[i].preferredType = VehicleType::CAR;
        } else {
            spots_[i].preferredType = VehicleType::VAN;
        }
    }
}

int ParkingLot::findBestSpot(VehicleType type) const {
    // 1. First search for a matching preferred spot
    for (size_t i = 0; i < spots_.size(); ++i) {
        if (spots_[i].vacant && spots_[i].preferredType == type) {
            return static_cast<int>(i);
        }
    }

    // 2. For special vehicles or overflow, search anywhere available
    for (size_t i = 0; i < spots_.size(); ++i) {
        if (spots_[i].vacant) {
            return static_cast<int>(i);
        }
    }

    return -1; // Lot is completely full
}

std::optional<ArrivalTicket> ParkingLot::arrival(VehicleType type, int vehicleId, const Date& date, int targetSpaceNumber) {
    // Check if vehicle ID is already parked
    if (findVehicle(vehicleId) != nullptr) {
        std::cerr << "[ParkingLot] Error: Vehicle #" << vehicleId << " is already inside the lot!\n";
        return std::nullopt;
    }

    int spotIdx = -1;
    if (targetSpaceNumber >= 0 && targetSpaceNumber < TOTAL_SPOTS) {
        if (spots_[targetSpaceNumber].vacant) {
            spotIdx = targetSpaceNumber;
        } else {
            std::cerr << "[ParkingLot] Error: Target spot #" << (targetSpaceNumber + 1) << " is already occupied!\n";
            return std::nullopt;
        }
    } else {
        spotIdx = findBestSpot(type);
    }

    if (spotIdx == -1) {
        std::cerr << "[ParkingLot] Error: Parking lot is full! Cannot park vehicle #" << vehicleId << ".\n";
        return std::nullopt;
    }

    auto& spot = spots_[spotIdx];
    spot.vacant = false;
    spot.vehicle.id = vehicleId;
    spot.vehicle.type = type;
    spot.vehicle.arrivalDate = date;
    spot.vehicle.floor = spot.floor;
    spot.vehicle.spaceNumber = spot.spaceNumber;
    spot.vehicle.active = true;

    ArrivalTicket ticket;
    ticket.vehicleId = vehicleId;
    ticket.vehicleType = type;
    ticket.arrivalDate = date;
    ticket.floor = spot.floor;
    ticket.spaceNumber = spot.spaceNumber;

    return ticket;
}

std::optional<DepartureTicket> ParkingLot::departure(int vehicleId, const Date& date) {
    for (auto& spot : spots_) {
        if (!spot.vacant && spot.vehicle.id == vehicleId) {
            DepartureTicket ticket;
            ticket.vehicleId = vehicleId;
            ticket.vehicleType = spot.vehicle.type;
            ticket.arrivalDate = spot.vehicle.arrivalDate;
            ticket.departureDate = date;
            ticket.floor = spot.floor;
            ticket.spaceNumber = spot.spaceNumber;

            int y = date.year - spot.vehicle.arrivalDate.year;
            int m = date.month - spot.vehicle.arrivalDate.month;
            int d = date.day - spot.vehicle.arrivalDate.day;

            if (d < 0) {
                m -= 1;
                d += 30;
            }
            if (m < 0) {
                y -= 1;
                m += 12;
            }
            if (y < 0) {
                y = 0; m = 0; d = 0;
            }

            ticket.yearsSpent = y;
            ticket.monthsSpent = m;
            ticket.daysSpent = d;
            ticket.totalDays = d + 30 * m + 365 * y;

            double rate = vehicleDailyRate(spot.vehicle.type);
            ticket.price = rate * std::max(1, ticket.totalDays);

            // Free spot
            spot.vacant = true;
            spot.vehicle.active = false;
            spot.vehicle.id = 0;

            return ticket;
        }
    }

    std::cerr << "[ParkingLot] Error: Vehicle #" << vehicleId << " not found in parking lot!\n";
    return std::nullopt;
}

bool ParkingLot::isSpotOccupied(int floor, int spaceNumber) const {
    int idx = toGlobalIndex(floor, spaceNumber);
    if (idx >= 0 && idx < TOTAL_SPOTS) {
        return !spots_[idx].vacant;
    }
    return false;
}

const Vehicle* ParkingLot::getVehicle(int floor, int spaceNumber) const {
    int idx = toGlobalIndex(floor, spaceNumber);
    if (idx >= 0 && idx < TOTAL_SPOTS && !spots_[idx].vacant) {
        return &spots_[idx].vehicle;
    }
    return nullptr;
}

const ParkingSpot* ParkingLot::getSpot(int floor, int spaceNumber) const {
    int idx = toGlobalIndex(floor, spaceNumber);
    if (idx >= 0 && idx < TOTAL_SPOTS) {
        return &spots_[idx];
    }
    return nullptr;
}

const ParkingSpot* ParkingLot::getSpotByIndex(int globalIndex) const {
    if (globalIndex >= 0 && globalIndex < TOTAL_SPOTS) {
        return &spots_[globalIndex];
    }
    return nullptr;
}

int ParkingLot::getOccupancy() const {
    int count = 0;
    for (const auto& s : spots_) {
        if (!s.vacant) count++;
    }
    return count;
}

std::vector<int> ParkingLot::getOccupiedSpotIndices(int floor) const {
    std::vector<int> res;
    for (int i = 0; i < SPOTS_PER_FLOOR; ++i) {
        if (isSpotOccupied(floor, i)) {
            res.push_back(i);
        }
    }
    return res;
}

std::vector<int> ParkingLot::getVacantSpotIndices(int floor) const {
    std::vector<int> res;
    for (int i = 0; i < SPOTS_PER_FLOOR; ++i) {
        if (!isSpotOccupied(floor, i)) {
            res.push_back(i);
        }
    }
    return res;
}

const Vehicle* ParkingLot::findVehicle(int vehicleId) const {
    for (const auto& s : spots_) {
        if (!s.vacant && s.vehicle.id == vehicleId) {
            return &s.vehicle;
        }
    }
    return nullptr;
}

void ParkingLot::printStatus() const {
    std::cout << "\n============================================\n"
              << "       PARKING LOT STATUS (" << getOccupancy() << "/" << TOTAL_SPOTS << " Occupied)\n"
              << "============================================\n";
    for (int s = 0; s < SPOTS_PER_FLOOR; ++s) {
        const auto& spot = spots_[s];
        std::cout << "  Spot [" << (s + 1) << "] (" << vehicleTypeName(spot.preferredType) << "): ";
        if (spot.vacant) {
            std::cout << "[ VACANT ]\n";
        } else {
            std::cout << "[ OCCUPIED ] Vehicle #" << spot.vehicle.id
                      << " (" << vehicleTypeName(spot.vehicle.type) << ")"
                      << " since " << spot.vehicle.arrivalDate.month << "/"
                      << spot.vehicle.arrivalDate.day << "/" << spot.vehicle.arrivalDate.year << "\n";
        }
    }
    std::cout << "============================================\n\n";
}
