#include "TestHarness.h"
#include "sim/Simulation.h"

TEST_CASE(Integration_GateQueue, EntranceChuteMeteringAndSequentialEntry) {
    Simulation sim;

    // Trigger 4 arrivals in rapid succession
    sim.triggerArrival(VehicleType::MOTORBIKE); // Vehicle 1
    sim.triggerArrival(VehicleType::MOTORBIKE); // Vehicle 2
    sim.triggerArrival(VehicleType::BICYCLE);    // Vehicle 3
    sim.triggerArrival(VehicleType::CAR);        // Vehicle 4

    // Initially: All 4 are queued at the entrance
    ASSERT_EQ(sim.getArrivalQueueSize(), 4);
    ASSERT_EQ(sim.getActiveAgents().size(), 0);

    // After first update step, Vehicle 1 enters the chute and 3 remain queued
    sim.update(0.016f);
    ASSERT_EQ(sim.getActiveAgents().size(), 1);
    ASSERT_EQ(sim.getArrivalQueueSize(), 3);

    // Run while Vehicle 1 traverses the entrance chute
    bool sawChuteHold = false;
    for (int frame = 0; frame < 80; ++frame) {
        sim.update(0.016f);

        if (!sim.getActiveAgents().empty()) {
            const auto& a = sim.getActiveAgents()[0];
            if (a.getPosition().getY() < -30.0f && a.getPosition().getX() > -104.0f && a.getPosition().getX() < -88.0f) {
                // While inside chute, queue must NOT release the next car!
                ASSERT_EQ(sim.getArrivalQueueSize(), 3);
                sawChuteHold = true;
            }
        }
    }
    ASSERT_TRUE(sawChuteHold);

    // Continue running until all 4 vehicles have completed parking
    int frames = 0;
    while ((!sim.getActiveAgents().empty() || sim.getArrivalQueueSize() > 0) && frames < 5000) {
        sim.update(0.016f);
        frames++;
    }

    ASSERT_EQ(sim.getArrivalQueueSize(), 0);
    ASSERT_EQ(sim.getActiveAgents().size(), 0);
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 4);
}
