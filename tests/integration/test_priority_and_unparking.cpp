#include "TestHarness.h"
#include "sim/Simulation.h"

TEST_CASE(Integration_Priority, BottomAisleMovingCarPriority) {
    Simulation sim;
    // Park vehicles in Bottom stalls 0..3
    sim.triggerArrival(VehicleType::MOTORBIKE); // Spot 0
    sim.triggerArrival(VehicleType::MOTORBIKE); // Spot 1
    sim.triggerArrival(VehicleType::BICYCLE);   // Spot 2
    sim.triggerArrival(VehicleType::BICYCLE);   // Spot 3 (ID 1004)

    // Wait until all 4 vehicles have fully parked
    int waitFrames = 0;
    while ((!sim.getActiveAgents().empty() || sim.getArrivalQueueSize() > 0) && waitFrames < 3000) {
        sim.update(0.016f);
        waitFrames++;
    }
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 4);
    int departingId = sim.getParkingLot().getVehicle(1, 3)->id;

    // Spawn a moving car destined for Stall 4 (Spot 5)
    sim.triggerArrival(VehicleType::CAR);
    int movingCarId = -1;
    // Advance until moving car enters bottom aisle driving East
    for (int f = 0; f < 500; ++f) {
        sim.update(0.016f);
        for (const auto& a : sim.getActiveAgents()) {
            if (a.getState() == CarState::DRIVING_AISLE_BOTTOM && a.getPosition().getX() > -60.0f) {
                movingCarId = a.getVehicleId();
                break;
            }
        }
        if (movingCarId != -1) break;
    }
    ASSERT_TRUE(movingCarId != -1);

    // Trigger departure for Stall 3 while moving car is approaching
    sim.triggerDeparture(departingId);

    bool movingCarStopped = false;
    bool unparkingCarStayedInStall = true;

    for (int f = 0; f < 250; ++f) {
        Vec prevPos;
        for (const auto& a : sim.getActiveAgents()) {
            if (a.getVehicleId() == movingCarId) prevPos = a.getPosition();
        }

        sim.update(0.016f);

        for (const auto& a : sim.getActiveAgents()) {
            if (a.getVehicleId() == movingCarId) {
                if (a.getState() == CarState::DRIVING_AISLE_BOTTOM && a.getPosition().getX() < 70.0f) {
                    float dist = (a.getPosition() - prevPos).length();
                    if (dist < 0.1f) {
                        movingCarStopped = true;
                    }
                }
            }
            if (a.getVehicleId() == departingId) {
                for (const auto& other : sim.getActiveAgents()) {
                    if (other.getVehicleId() == movingCarId && other.getPosition().getX() < a.getPosition().getX() + 20.0f) {
                        if (a.getPosition().getY() > -49.0f) {
                            unparkingCarStayedInStall = false;
                        }
                    }
                }
            }
        }
    }

    ASSERT_FALSE(movingCarStopped);
    ASSERT_TRUE(unparkingCarStayedInStall);
}

TEST_CASE(Integration_Priority, TopAisleMovingCarPriority) {
    Simulation sim;
    // Fill lot up to 9 spots so top stalls are occupied
    for (int i = 0; i < 9; ++i) {
        sim.triggerArrival((i < 2) ? VehicleType::MOTORBIKE : ((i < 4) ? VehicleType::BICYCLE : VehicleType::CAR));
    }
    int waitFrames = 0;
    while ((!sim.getActiveAgents().empty() || sim.getArrivalQueueSize() > 0) && waitFrames < 5000) {
        sim.update(0.016f);
        waitFrames++;
    }
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 9);
    int departingTopId = sim.getParkingLot().getVehicle(1, 7)->id; // Spot 8

    // Trigger arrival for Spot 9
    sim.triggerArrival(VehicleType::CAR);
    int topMovingCarId = -1;
    for (int f = 0; f < 1000; ++f) {
        sim.update(0.016f);
        for (const auto& a : sim.getActiveAgents()) {
            if (a.getState() == CarState::DRIVING_AISLE_TOP && a.getPosition().getX() > 30.0f) {
                topMovingCarId = a.getVehicleId();
                break;
            }
        }
        if (topMovingCarId != -1) break;
    }
    ASSERT_TRUE(topMovingCarId != -1);

    // Trigger departure for top stall 7 while moving car is driving West
    sim.triggerDeparture(departingTopId);

    bool topMovingCarStopped = false;
    bool topUnparkingStayedInStall = true;

    for (int f = 0; f < 250; ++f) {
        Vec prevPos;
        for (const auto& a : sim.getActiveAgents()) {
            if (a.getVehicleId() == topMovingCarId) prevPos = a.getPosition();
        }

        sim.update(0.016f);

        for (const auto& a : sim.getActiveAgents()) {
            if (a.getVehicleId() == topMovingCarId) {
                if (a.getState() == CarState::DRIVING_AISLE_TOP && a.getPosition().getX() > -60.0f) {
                    float dist = (a.getPosition() - prevPos).length();
                    if (dist < 0.1f) {
                        topMovingCarStopped = true;
                    }
                }
            }
            if (a.getVehicleId() == departingTopId) {
                for (const auto& other : sim.getActiveAgents()) {
                    if (other.getVehicleId() == topMovingCarId && other.getPosition().getX() > a.getPosition().getX() - 20.0f) {
                        if (a.getPosition().getY() < 79.0f) {
                            topUnparkingStayedInStall = false;
                        }
                    }
                }
            }
        }
    }

    ASSERT_FALSE(topMovingCarStopped);
    ASSERT_TRUE(topUnparkingStayedInStall);
}
