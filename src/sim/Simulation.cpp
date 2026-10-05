#include "Simulation.h"
#include <iostream>
#include <cstdlib>
#include <cmath>

Simulation::Simulation() {
    // Starts with an empty parking lot
}

void Simulation::logMessage(const std::string& msg) {
    std::cout << msg << std::flush;
    ticketLog_.push_back(msg);
    if (ticketLog_.size() > 50) {
        ticketLog_.pop_front();
    }
}

bool Simulation::isVehicleDriving(int vehicleId) const {
    for (const auto& q : arrivalQueue_) {
        if (q.vehicleId == vehicleId) {
            return true;
        }
    }
    for (const auto& a : activeAgents_) {
        if (a.getVehicleId() == vehicleId && !a.isParked() && !a.isFinished()) {
            return true;
        }
    }
    return false;
}

void Simulation::triggerArrival(VehicleType type, int targetSpot) {
    if (targetSpot >= 0) {
        if (parkingLot_.isSpotOccupied(1, targetSpot)) {
            logMessage("[SIM] Arrival failed: Spot " + std::to_string(targetSpot + 1) + " is already occupied or assigned!\n");
            return;
        }
    }

    int id = nextVehicleId_++;
    auto ticketOpt = parkingLot_.arrival(type, id, currentDate_, targetSpot);
    if (!ticketOpt.has_value()) {
        logMessage("[SIM] Arrival failed: No available spots for " + vehicleTypeName(type) + "\n");
        return;
    }

    const auto& ticket = ticketOpt.value();
    logMessage(ticket.toString());

    arrivalQueue_.push_back({id, type, ticket.spaceNumber, ticket.floor});
    if (arrivalQueue_.size() > 1) {
        logMessage("[SIM] Vehicle #" + std::to_string(id) + " queued at entrance gate.\n");
    }
}

void Simulation::triggerDeparture(int vehicleId) {
    if (vehicleId == -1) {
        auto occupied = parkingLot_.getOccupiedSpotIndices(1);
        for (int spot : occupied) {
            const auto* veh = parkingLot_.getVehicle(1, spot);
            if (veh && !isVehicleDeparting(veh->id) && !isVehicleDriving(veh->id)) {
                vehicleId = veh->id;
                break;
            }
        }
    }

    if (vehicleId == -1) {
        logMessage("[SIM] Departure failed: No parked vehicles available to depart!\n");
        return;
    }

    if (isVehicleDeparting(vehicleId)) {
        return; // Already departing
    }

    if (isVehicleDriving(vehicleId)) {
        logMessage("[SIM] Vehicle #" + std::to_string(vehicleId) + " is still driving to park!\n");
        return;
    }

    const auto* veh = parkingLot_.findVehicle(vehicleId);
    if (!veh) {
        logMessage("[SIM] Vehicle #" + std::to_string(vehicleId) + " not found in lot!\n");
        return;
    }

    int vehFloor = veh->floor;
    int vehSpot = veh->spaceNumber;
    VehicleType vehType = veh->type;

    departingVehicleIds_.insert(vehicleId);

    // Check if an agent already exists in activeAgents_ for this vehicle
    bool agentFound = false;
    for (auto& agent : activeAgents_) {
        if (agent.getVehicleId() == vehicleId) {
            agent.startDeparture();
            agentFound = true;
            break;
        }
    }

    // If vehicle was parked without an active agent, instantiate one to execute departure animation
    if (!agentFound) {
        CarAgent agent(vehicleId, vehType, vehSpot, vehFloor);
        agent.startDeparture();
        activeAgents_.push_back(agent);
    }
}

void Simulation::stepAutoSimulation() {
    int roll = rand() % 100;
    if (roll < 60) {
        // 60% chance: Arrival
        int typeRoll = rand() % 4;
        VehicleType type = VehicleType::CAR;
        if (typeRoll == 0) type = VehicleType::VAN;
        else if (typeRoll == 1) type = VehicleType::CAR;
        else if (typeRoll == 2) type = VehicleType::MOTORBIKE;
        else if (typeRoll == 3) type = VehicleType::BICYCLE;

        triggerArrival(type);
    } else {
        // 40% chance: Departure
        triggerDeparture();
    }
}

void Simulation::update(float deltaTime) {
    // 1. Process queued arrivals when entrance gate area is clear
    if (!arrivalQueue_.empty()) {
        bool entranceClear = true;
        for (const auto& agent : activeAgents_) {
            if (!agent.isMoving()) continue;
            // Block only if an active agent is in the entrance chute (x in (-104, -88), y < -30)
            if (agent.getPosition().getY() < -30.0f && agent.getPosition().getX() > -104.0f && agent.getPosition().getX() < -88.0f) {
                entranceClear = false;
                break;
            }
        }
        if (entranceClear) {
            auto next = arrivalQueue_.front();
            arrivalQueue_.pop_front();
            CarAgent agent(next.vehicleId, next.type, next.targetSpot, next.floor);
            agent.startParking();
            activeAgents_.push_back(agent);
        }
    }

    // 2. Advance auto simulation timer
    if (autoMode_) {
        autoTimer_ += deltaTime;
        if (autoTimer_ >= autoInterval_) {
            autoTimer_ = 0.0f;
            stepAutoSimulation();
        }
    }

    // 3. Update active driving agents with inter-vehicle collision avoidance
    auto currentAgents = activeAgents_;
    for (auto it = activeAgents_.begin(); it != activeAgents_.end();) {
        it->update(currentAgents);
        if (it->isParked()) {
            it = activeAgents_.erase(it);
        } else if (it->isFinished()) {
            int vehId = it->getVehicleId();
            Date depDate = currentDate_;
            depDate.day += (1 + rand() % 5);
            auto ticketOpt = parkingLot_.departure(vehId, depDate);
            if (ticketOpt.has_value()) {
                logMessage(ticketOpt.value().toString());
            }
            departingVehicleIds_.erase(vehId);
            it = activeAgents_.erase(it);
        } else {
            ++it;
        }
    }

    animFrame_++;
}

void Simulation::renderParkedVehicles(RenderBatch& batch) {
    for (int s = 0; s < ParkingLot::SPOTS_PER_FLOOR; ++s) {
        if (!parkingLot_.isSpotOccupied(currentFloor_, s)) {
            // Draw green vacant indicator in stall
            float spotX = (s <= 4) ? (-68.0f + 34.0f * s) : (238.0f - 34.0f * s);
            float spotY = (s <= 4) ? -50.0f : 80.0f;
            batch.addCircle(spotX, spotY, 2.5f, Color(0.2f, 0.8f, 0.2f, 0.8f));
            continue;
        }

        const auto* veh = parkingLot_.getVehicle(currentFloor_, s);
        if (!veh) continue;

        // Skip if this vehicle is currently driving or queued to drive
        if (isVehicleDriving(veh->id) || isVehicleDeparting(veh->id)) continue;

        // Render parked vehicle based on type
        if (veh->type == VehicleType::BICYCLE || veh->type == VehicleType::MOTORBIKE) {
            // Compact two-wheeler style representation
            float spotX = (s <= 4) ? (-68.0f + 34.0f * s) : (238.0f - 34.0f * s);
            float spotY = (s <= 4) ? -50.0f : 80.0f;
            Color bikeColor = (veh->type == VehicleType::MOTORBIKE) ? Color(0.9f, 0.5f, 0.1f) : Color(0.1f, 0.7f, 0.8f);
            batch.addRect(spotX - 4.0f, spotY - 8.0f, spotX + 4.0f, spotY + 8.0f, bikeColor);
            batch.addCircle(spotX, spotY - 7.0f, 2.5f, Color::Black());
            batch.addCircle(spotX, spotY + 7.0f, 2.5f, Color::Black());
        } else {
            // Car / Van
            Vec carPos = (s <= 4) ? Vec(-68.0f + 34.0f * s, -50.0f) : Vec(238.0f - 34.0f * s, 80.0f);
            Vec carAnc = (s <= 4) ? Vec(0.0f, 0.4f) : Vec(0.0f, -0.4f);
            CarFig car(carPos, carAnc);
            car.buildVertices(batch);
        }
    }
}

void Simulation::buildRenderBatch(RenderBatch& batch) {
    batch.clear();

    // 1. Draw parking stall structure and outer walls
    ParkingLotFig::buildVertices(batch);

    // 2. Draw parked vehicles
    renderParkedVehicles(batch);

    // 3. Draw active navigating vehicles
    for (const auto& agent : activeAgents_) {
        agent.buildVertices(batch);
    }
}

void Simulation::resetLot() {
    arrivalQueue_.clear();
    activeAgents_.clear();
    departingVehicleIds_.clear();
    parkingLot_ = ParkingLot();
    nextVehicleId_ = 1;
    logMessage("[SIM] Parking lot reset to empty state.\n");
}

