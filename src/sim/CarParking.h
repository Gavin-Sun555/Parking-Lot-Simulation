#ifndef SIM_CAR_PARKING_H
#define SIM_CAR_PARKING_H

#include "Vec.h"
#include "Figures.h"
#include "../core/Vehicle.h"
#include "../render/RenderTypes.h"

enum class CarState {
    IDLE = 0,
    ENTERING = 1,
    TURNING_AISLE = 2,
    DRIVING_AISLE_BOTTOM = 3,
    TURNING_LOOP = 4,
    DRIVING_AISLE_TOP = 5,
    ALIGNING_BOTTOM = 6,
    PARKING_BOTTOM = 7,
    ALIGNING_TOP = 8,
    PARKING_TOP = 9,
    PARKED = 10,
    UNPARKING_BOTTOM_PULLOUT = 11,
    UNPARKING_BOTTOM_TURN = 12,
    DRIVING_AISLE_BOTTOM_EXIT = 13,
    TURNING_GATE_BOTTOM = 14,
    UNPARKING_TOP_PULLOUT = 15,
    UNPARKING_TOP_TURN = 16,
    DRIVING_AISLE_TOP_EXIT = 17,
    TURNING_CORRIDOR_TOP = 18,
    DRIVING_CORRIDOR_SOUTH = 19,
    EXITING_GATE = 20,
    DEPARTED = 21
};

class CarAgent {
public:
    CarAgent();
    CarAgent(int vehicleId, VehicleType type, int targetSpot, int floor);

    int getVehicleId() const { return vehicleId_; }
    VehicleType getType() const { return type_; }
    int getTargetSpot() const { return targetSpot_; }
    int getFloor() const { return floor_; }
    CarState getState() const { return state_; }
    bool isParked() const { return state_ == CarState::PARKED; }
    bool isDeparting() const { return isDeparting_; }
    bool isFinished() const { return state_ == CarState::DEPARTED; }
    bool isMoving() const {
        return state_ != CarState::IDLE && state_ != CarState::PARKED && state_ != CarState::DEPARTED;
    }

    void startParking();
    void startDeparture();

    bool shouldYield(const std::vector<CarAgent>& allAgents) const;
    bool isAisleClear(const std::vector<CarAgent>& allAgents) const;

    void update(const std::vector<CarAgent>& allAgents);
    void buildVertices(RenderBatch& batch) const;

    Vec getPosition() const { return pos_; }
    Vec getAnchor() const { return anc_; }
    int getYieldFrames() const { return yieldFrames_; }

private:
    int vehicleId_ = 0;
    VehicleType type_ = VehicleType::CAR;
    int targetSpot_ = 0;
    int floor_ = 1;
    CarState state_ = CarState::IDLE;
    int yieldFrames_ = 0;
    bool isDeparting_ = false;
    float progress_ = 0.0f;

    Vec pos_;
    Vec anc_;
    CarFig carFig_;

    void stepParking(const std::vector<CarAgent>& allAgents);
    void stepDeparture(const std::vector<CarAgent>& allAgents);
};

#endif // SIM_CAR_PARKING_H
