#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#include <vector>
#include <optional>
#include <string>
#include "Vehicle.h"
#include "Ticket.h"

struct ParkingSpot {
    int floor = 1;
    int spaceNumber = 0; // 0 to 9
    VehicleType preferredType = VehicleType::CAR;
    bool vacant = true;
    Vehicle vehicle;
};

class ParkingLot {
public:
    static constexpr int SPOTS_PER_FLOOR = 10;
    static constexpr int NUM_FLOORS = 1;
    static constexpr int TOTAL_SPOTS = SPOTS_PER_FLOOR * NUM_FLOORS; // 10

    ParkingLot();
    ~ParkingLot() = default;

    // Issue ticket and park vehicle
    std::optional<ArrivalTicket> arrival(VehicleType type, int vehicleId, const Date& date, int targetSpaceNumber = -1);

    // Process departure and return ticket
    std::optional<DepartureTicket> departure(int vehicleId, const Date& date);

    // Query state
    bool isSpotOccupied(int floor, int spaceNumber) const;
    const Vehicle* getVehicle(int floor, int spaceNumber) const;
    const ParkingSpot* getSpot(int floor, int spaceNumber) const;
    const ParkingSpot* getSpotByIndex(int globalIndex) const;

    int getOccupancy() const;
    int getCapacity() const { return TOTAL_SPOTS; }
    std::vector<int> getOccupiedSpotIndices(int floor) const;
    std::vector<int> getVacantSpotIndices(int floor) const;

    // Find vehicle by ID
    const Vehicle* findVehicle(int vehicleId) const;

    void printStatus() const;

private:
    std::vector<ParkingSpot> spots_;

    int findBestSpot(VehicleType type) const;
    static int toGlobalIndex(int floor, int spaceNumber) {
        return (floor - 1) * SPOTS_PER_FLOOR + spaceNumber;
    }
};

#endif // PARKING_LOT_H
