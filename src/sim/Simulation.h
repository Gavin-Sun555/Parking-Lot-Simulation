#ifndef SIM_SIMULATION_H
#define SIM_SIMULATION_H

#include "../core/ParkingLot.h"
#include "CarParking.h"
#include "Figures.h"
#include "../render/RenderTypes.h"
#include <vector>
#include <deque>
#include <set>
#include <memory>

class Simulation {
public:
    Simulation();

    void update(float deltaTime);
    void buildRenderBatch(RenderBatch& batch);

    // Interactive commands
    void triggerArrival(VehicleType type = VehicleType::CAR, int targetSpot = -1);
    void triggerDeparture(int vehicleId = -1);
    int getCurrentFloor() const { return 1; }

    bool isAutoMode() const { return autoMode_; }
    void setAutoMode(bool a) { autoMode_ = a; }
    void toggleAutoMode() { autoMode_ = !autoMode_; }

    float getAutoInterval() const { return autoInterval_; }
    void setAutoInterval(float interval) { autoInterval_ = (interval > 0.5f) ? interval : 0.5f; }

    const ParkingLot& getParkingLot() const { return parkingLot_; }
    ParkingLot& getParkingLot() { return parkingLot_; }

    const std::deque<std::string>& getTicketLog() const { return ticketLog_; }
    void logMessage(const std::string& msg);
    void clearLog() { ticketLog_.clear(); }
    void resetLot();

    size_t getArrivalQueueSize() const { return arrivalQueue_.size(); }
    bool isVehicleDeparting(int vehicleId) const { return departingVehicleIds_.count(vehicleId) > 0; }
    bool isVehicleDriving(int vehicleId) const;
    const std::vector<CarAgent>& getActiveAgents() const { return activeAgents_; }


private:
    struct QueuedArrival {
        int vehicleId;
        VehicleType type;
        int targetSpot;
        int floor;
    };

    ParkingLot parkingLot_;
    int currentFloor_ = 1;
    bool autoMode_ = false;

    float autoTimer_ = 0.0f;
    float autoInterval_ = 4.0f; // seconds between events
    int nextVehicleId_ = 1;
    Date currentDate_{2026, 10, 4};

    // Active driving vehicles
    std::vector<CarAgent> activeAgents_;
    std::deque<QueuedArrival> arrivalQueue_;
    std::set<int> departingVehicleIds_;

    int animFrame_ = 0;

    std::deque<std::string> ticketLog_;

    void stepAutoSimulation();
    void renderParkedVehicles(RenderBatch& batch);
};

#endif // SIM_SIMULATION_H
